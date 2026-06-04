#pragma once
#include <string>
#include <atomic>
#include <thread>
#include <chrono>
#include <fstream>
#include <csignal>
#include "config.h"
#include "http_client.h"
#include "task_executor.h"
#include "logger.h"
#include <nlohmann/json.hpp>

class Agent {
public:
    explicit Agent(const Config& cfg, const std::string& config_path = "")
        : cfg_(cfg),
          http_(cfg.ssl_verify),
          executor_(cfg.tasks_dir, cfg.results_dir, config_path),
          running_(false),
          poll_interval_(cfg.poll_interval_sec) {}

    void run() {
        running_ = true;
        LOG_INFO("=== Web-Agent v1.0 (UID: " + cfg_.uid + ") ===");

        wait_for_server();

        if (!register_agent()) {
            LOG_ERROR("Registration failed. Exiting.");
            return;
        }

        while (running_) {
            try {
                poll_task();
            } catch (const std::exception& e) {
                LOG_ERROR(std::string("Poll exception: ") + e.what());
                backoff();
            }
            std::this_thread::sleep_for(std::chrono::seconds(poll_interval_));
        }

        LOG_INFO("=== Web-Agent stopped ===");
    }

    void stop() { running_ = false; }

private:
    Config        cfg_;
    HttpClient    http_;
    TaskExecutor  executor_;
    std::atomic<bool> running_;
    int           poll_interval_;
    std::string   access_code_;

    void wait_for_server() {
        while (running_) {
            LOG_INFO("Checking server: " + cfg_.server_url);
            if (http_.is_reachable(cfg_.server_url + "/wa_reg/")) {
                LOG_INFO("Server reachable.");
                poll_interval_ = cfg_.poll_interval_sec;
                return;
            }
            LOG_WARN("Server unreachable. Retry in " + std::to_string(poll_interval_) + "s");
            std::this_thread::sleep_for(std::chrono::seconds(poll_interval_));
            backoff();
        }
    }

    void backoff() {
        poll_interval_ = std::min(poll_interval_ * 2, cfg_.max_poll_interval_sec);
        LOG_INFO("Backoff: next poll in " + std::to_string(poll_interval_) + "s");
    }

    void reset_interval() { poll_interval_ = cfg_.poll_interval_sec; }

    bool register_agent() {
        std::string url = cfg_.server_url + "/wa_reg/";
        nlohmann::json req = {{"UID", cfg_.uid}, {"descr", cfg_.descr}};

        LOG_INFO("Registering at: " + url);
        try {
            auto resp = http_.post_json(url, req);
            LOG_DEBUG("Register [" + std::to_string(resp.status_code) + "]: " + resp.body);

            auto j = nlohmann::json::parse(resp.body);
            int code = std::stoi(j.value("code_responce", "-99"));

            if (code == 0) {
                access_code_ = j.value("access_code", "");
                save_access_code();
                LOG_INFO("Registered. access_code: " + access_code_);
                return true;
            } else if (code == -3) {
                LOG_WARN("Already registered. Using cached access_code.");
                load_access_code();
                return !access_code_.empty();
            } else {
                LOG_ERROR("Registration error: " + j.value("msg", "?"));
                return false;
            }
        } catch (const std::exception& e) {
            LOG_ERROR(std::string("Register exception: ") + e.what());
            return false;
        }
    }

    void load_access_code() {
        std::ifstream f(cfg_.tasks_dir + "/.access_code");
        if (f.is_open()) std::getline(f, access_code_);
    }

    void save_access_code() {
        std::ofstream f(cfg_.tasks_dir + "/.access_code");
        if (f.is_open()) f << access_code_;
    }

    void poll_task() {
        std::string url = cfg_.server_url + "/wa_task/";
        nlohmann::json req = {
            {"UID",         cfg_.uid},
            {"descr",       cfg_.descr},
            {"access_code", access_code_}
        };

        LOG_DEBUG("Polling...");
        auto resp = http_.post_json(url, req);

        if (resp.status_code == 0) { backoff(); return; }

        LOG_DEBUG("Task [" + std::to_string(resp.status_code) + "]: " + resp.body);

        nlohmann::json j;
        try { j = nlohmann::json::parse(resp.body); }
        catch (...) { LOG_ERROR("Bad JSON: " + resp.body); return; }

        int code = std::stoi(j.value("code_responce", "0"));

        if (code == 0) {
            LOG_DEBUG("No task (WAIT)");
            reset_interval();
            return;
        }
        if (code < 0) {
            LOG_ERROR("Task error: " + j.value("msg", "?"));
            if (code == -2) { LOG_WARN("Bad access_code, re-registering..."); register_agent(); }
            return;
        }

        // code == 1: task available
        std::string task_code  = j.value("task_code",  "");
        std::string options    = j.value("options",    "");
        std::string session_id = j.value("session_id", "");

        LOG_INFO("Task: code=" + task_code + " session=" + session_id);
        reset_interval();

        TaskResult result = executor_.execute(task_code, options, session_id);

        // Apply new interval if SET_INTERVAL task
        int new_iv = executor_.get_new_interval();
        if (new_iv > 0) {
            LOG_INFO("Applying new poll interval: " + std::to_string(new_iv) + "s");
            poll_interval_ = new_iv;
            cfg_.poll_interval_sec = new_iv;
        }

        send_result(session_id, result);
    }

    void send_result(const std::string& session_id, const TaskResult& result) {
        std::string url = cfg_.server_url + "/wa_result/";

        nlohmann::json meta = {
            {"UID",         cfg_.uid},
            {"access_code", access_code_},
            {"message",     result.message},
            {"files",       (int)result.result_files.size()},
            {"session_id",  session_id}
        };

        std::map<std::string, std::string> fields;
        fields["result_code"] = std::to_string(result.exit_code);
        fields["result"]      = meta.dump();

        std::vector<UploadFile> uploads;
        for (size_t i = 0; i < result.result_files.size(); ++i) {
            uploads.push_back({"file" + std::to_string(i + 1), result.result_files[i]});
        }

        LOG_INFO("Sending result: exit=" + std::to_string(result.exit_code)
                 + " files=" + std::to_string(uploads.size()));

        try {
            auto resp = http_.post_multipart(url, fields, uploads);
            LOG_DEBUG("Result response [" + std::to_string(resp.status_code) + "]: " + resp.body);
            auto j = nlohmann::json::parse(resp.body);
            int c = std::stoi(j.value("code_responce", "-99"));
            if (c == 0)
                LOG_INFO("Result accepted: " + j.value("msg", "ok"));
            else
                LOG_ERROR("Result rejected: " + j.value("msg", "?") + " status=" + j.value("status",""));
        } catch (const std::exception& e) {
            LOG_ERROR(std::string("Send result exception: ") + e.what());
        }
    }
};
