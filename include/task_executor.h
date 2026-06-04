#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include "logger.h"

namespace fs = std::filesystem;

struct TaskResult {
    int         exit_code{0};
    std::string message;
    std::vector<std::string> result_files;
};

class TaskExecutor {
public:
    explicit TaskExecutor(const std::string& tasks_dir, const std::string& results_dir,
                          const std::string& config_path = "")
        : tasks_dir_(tasks_dir), results_dir_(results_dir), config_path_(config_path) {
        fs::create_directories(tasks_dir_);
        fs::create_directories(results_dir_);
    }

    // Returns new poll interval if SET_INTERVAL task, -1 otherwise
    int get_new_interval() const { return new_interval_; }

    TaskResult execute(const std::string& task_code, const std::string& options_str,
                       const std::string& session_id) {
        new_interval_ = -1;
        TaskResult res;

        LOG_INFO("Executing task: " + task_code + " session: " + session_id);

        nlohmann::json opts;
        if (!options_str.empty()) {
            try { opts = nlohmann::json::parse(options_str); }
            catch (...) { opts["command"] = options_str; }
        }

        if (task_code == "CONF") {
            res = execute_conf(opts, session_id);
        } else if (task_code == "RUN") {
            res = execute_run(opts, session_id);
        } else if (task_code == "UPLOAD" || task_code == "FILE" || task_code == "GET_FILE") {
            res = collect_results(session_id);
        } else if (task_code == "SET_INTERVAL" || task_code == "INTERVAL") {
            res = execute_set_interval(opts, session_id);
        } else if (task_code == "SET_PARAM" || task_code == "CONFIG") {
            res = execute_set_param(opts, session_id);
        } else {
            // Fallback: treat as shell command
            std::string cmd = opts.value("command", task_code);
            res = run_command(cmd, session_id);
        }

        return res;
    }

private:
    std::string tasks_dir_;
    std::string results_dir_;
    std::string config_path_;
    int         new_interval_{-1};

    // ---- CONF: run a shell command ----
    TaskResult execute_conf(const nlohmann::json& opts, const std::string& session_id) {
        std::string command = opts.value("command", "");
        if (command.empty()) {
            TaskResult res;
            res.exit_code = -1;
            res.message = "CONF task: no 'command' in options";
            LOG_WARN(res.message);
            return res;
        }
        return run_command(command, session_id);
    }

    // ---- RUN: run a program ----
    TaskResult execute_run(const nlohmann::json& opts, const std::string& session_id) {
        std::string program = opts.value("program", opts.value("command", ""));
        std::string args    = opts.value("args", "");
        if (program.empty()) {
            TaskResult res; res.exit_code = -1; res.message = "RUN: no program";
            return res;
        }
        std::string full_cmd = program;
        if (!args.empty()) full_cmd += " " + args;
        return run_command(full_cmd, session_id);
    }

    // ---- SET_INTERVAL: change poll interval ----
    TaskResult execute_set_interval(const nlohmann::json& opts, const std::string& session_id) {
        TaskResult res;
        int interval = opts.value("interval", opts.value("value", -1));
        if (interval <= 0) {
            res.exit_code = -1;
            res.message = "SET_INTERVAL: invalid or missing 'interval' value";
            LOG_WARN(res.message);
            return res;
        }
        new_interval_ = interval;
        res.message = "Poll interval changed to " + std::to_string(interval) + "s";
        LOG_INFO(res.message);

        // Write result file
        std::string out = results_dir_ + "/interval_" + sanitize(session_id) + ".txt";
        std::ofstream f(out);
        if (f.is_open()) { f << res.message << "\n"; res.result_files.push_back(out); }

        // Also update config file if known
        if (!config_path_.empty()) update_config_interval(interval);

        return res;
    }

    // ---- SET_PARAM: change a config parameter ----
    TaskResult execute_set_param(const nlohmann::json& opts, const std::string& session_id) {
        TaskResult res;
        std::string param = opts.value("param", opts.value("key", ""));
        std::string value = opts.value("value", "");

        if (param.empty()) {
            res.exit_code = -1;
            res.message = "SET_PARAM: missing 'param' key";
            return res;
        }

        std::string out = results_dir_ + "/param_" + sanitize(session_id) + ".txt";
        std::ofstream f(out);
        if (f.is_open()) {
            f << "param=" << param << "\nvalue=" << value << "\n";
            res.result_files.push_back(out);
        }

        // Update config.json if path is known
        if (!config_path_.empty()) {
            try {
                std::ifstream ci(config_path_);
                nlohmann::json cfg; ci >> cfg;
                cfg[param] = value;
                std::ofstream co(config_path_);
                co << cfg.dump(4);
                res.message = "Config param '" + param + "' set to '" + value + "'";
                LOG_INFO(res.message);
            } catch (...) {
                res.message = "Param noted but config update failed";
            }
        } else {
            res.message = "Param '" + param + "'='" + value + "' noted (no config path)";
        }

        return res;
    }

    // ---- COLLECT: gather all files in results_dir ----
    TaskResult collect_results(const std::string& /*session_id*/) {
        TaskResult res;
        res.message = "Collecting result files";
        for (auto& entry : fs::directory_iterator(results_dir_)) {
            if (entry.is_regular_file())
                res.result_files.push_back(entry.path().string());
        }
        if (res.result_files.empty()) {
            // Create a placeholder so server gets something
            std::string out = results_dir_ + "/no_files.txt";
            std::ofstream f(out); f << "No result files available.\n";
            res.result_files.push_back(out);
        }
        res.exit_code = 0;
        return res;
    }

    // ---- Shell command runner ----
    TaskResult run_command(const std::string& cmd, const std::string& session_id) {
        TaskResult res;
        std::string out_file = results_dir_ + "/output_" + sanitize(session_id) + ".txt";
        std::string full_cmd = cmd + " > " + out_file + " 2>&1";

        LOG_INFO("Running: " + cmd);
        int rc = std::system(full_cmd.c_str());

        res.exit_code = rc;
        res.message   = (rc == 0) ? "OK" : "Exit code " + std::to_string(rc);
        LOG_INFO("Command finished: " + res.message);

        if (fs::exists(out_file)) res.result_files.push_back(out_file);

        for (auto& e : fs::directory_iterator(results_dir_)) {
            if (e.is_regular_file() && e.path().string() != out_file)
                res.result_files.push_back(e.path().string());
        }
        return res;
    }

    void update_config_interval(int interval) {
        try {
            std::ifstream ci(config_path_);
            nlohmann::json cfg; ci >> cfg;
            cfg["poll_interval_sec"] = interval;
            std::ofstream co(config_path_);
            co << cfg.dump(4);
            LOG_INFO("Config updated: poll_interval_sec=" + std::to_string(interval));
        } catch (...) {
            LOG_WARN("Could not update config file for interval change");
        }
    }

    std::string sanitize(const std::string& s) {
        std::string out;
        for (char c : s) if (std::isalnum(c) || c=='-' || c=='_') out+=c;
        return out.empty() ? "sess" : out;
    }
};
