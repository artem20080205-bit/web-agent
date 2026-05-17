#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <filesystem>
#include <ctime>
#include <sstream>

#include "httplib.h"
#include <nlohmann/json.hpp>
#include "ini.h"

using json = nlohmann::json;

class Client {
private:
    std::string uid;
    std::string server_host;
    int server_port;
    std::string base_path;
    std::string access_code;
    std::string log_level = "info";

public:
    Client(const std::string &config_path);
    void setConfig(const std::string& key, const std::string& value);
    bool isValid() const;
    std::vector<json> fetchServerTasks();
    bool sendResultsToServer(const json& result);
    void log(const std::string& msg, const std::string& level = "info");
    bool registerAgent();
};

class WebAgent {
private:
    std::string uid;
    std::string server_host;
    int server_port;
    std::string base_path;
    std::string access_code;
    std::string tasks_dir;
    std::string results_dir;
    int task_interval = 30;
    std::string log_level = "info";
    Client client;

public:
    WebAgent(const std::string& config_path);
    void run();
    void setConfig(const std::string& key, const std::string& value);
    void log(const std::string& msg, const std::string& level = "info");
    void executeTask(const json& task);
    void checkTasks();
};

void Client::log(const std::string& msg, const std::string& level) {
    static const std::vector<std::string> levels = {"debug", "info", "warning", "error"};

    auto lvl = [&](const std::string& l) -> int {
        for (size_t i = 0; i < levels.size(); i++) {
            if (levels[i] == l) return static_cast<int>(i);
        }
        return -1;
    };

    if (lvl(level) < lvl(log_level)) return;

    std::error_code ec;
    std::filesystem::create_directories("logs", ec);

    std::time_t now = std::time(nullptr);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%F %T", std::localtime(&now));

    std::string line = "[" + std::string(buf) + "] [" + level + "] " + msg;

    std::ofstream f("logs/client.log", std::ios::app);
    if (f.is_open()) {
        f << line << "\n";
        f.close();
    }

    std::cout << line << std::endl;
}

Client::Client(const std::string &config_path) {
    if (!std::filesystem::exists(config_path)) {
        throw std::runtime_error("Client config file not found: " + config_path);
    }

    auto client_handler = [](void* user, const char* section, const char* name, const char* value) -> int {
        if (!user) return 0;
       
        if (std::string(section) == "client") {
            Client* client = static_cast<Client*>(user);
            client->setConfig(name, value);
        }
        return 1;
    };

    int result = ini_parse(config_path.c_str(), client_handler, this);
    if (result != 0) {
        std::cerr << "Client config parse error: " << result << std::endl;
        throw std::runtime_error("Client config load failed");
    }

    log("Client initialized with UID: " + uid, "info");
    if (access_code.empty()) {
        log("No access_code, attempting registration...", "info");
        registerAgent();
    } else {
        log("Using existing access_code: " + access_code, "info");
    }
}

bool Client::registerAgent() {
    log("Registering agent...", "info");
    
    std::string full_url = "https://" + server_host + ":" + std::to_string(server_port);
    httplib::Client cli(full_url);
    cli.enable_server_certificate_verification(false);
    cli.set_connection_timeout(10, 0);
    cli.set_read_timeout(10, 0);
    
    json request = {
        {"UID", uid},
        {"descr", "web-agent"}
    };
    
    std::string url = base_path + "/wa_reg/";
    auto res = cli.Post(url.c_str(), request.dump(), "application/json");
    
    if (!res) {
        log("Registration failed: no response", "error");
        return false;
    }
    
    if (res->status != 200) {
        log("Registration failed: HTTP " + std::to_string(res->status), "error");
        return false;
    }
    
    try {
        json response = json::parse(res->body);
        std::string code = response.value("code_responce", "");
        
        if (code == "0") {
            access_code = response.value("access_code", "");
            log("Registration successful, access_code: " + access_code, "info");
            return true;
        } else if (code == "-3") {
            log("Agent already registered", "warning");
            return true;
        } else {
            log("Registration failed with code: " + code, "error");
            return false;
        }
    } catch (const std::exception& e) {
        log("Registration failed: invalid JSON response", "error");
        return false;
    }
}

void Client::setConfig(const std::string& key, const std::string& value) {
    if (value.empty()) return;

    if (key == "uid") uid = value;
    else if (key == "server_host") server_host = value;
    else if (key == "server_port") server_port = std::stoi(value);
    else if (key == "base_path") base_path = value;
    else if (key == "access_code") access_code = value;
    else if (key == "log_level") log_level = value;
}

bool Client::isValid() const {
    return !uid.empty() && !server_host.empty() && server_port > 0 && !access_code.empty();
}

std::vector<json> Client::fetchServerTasks() {
    if (server_host.empty()) {
        log("Server host is empty", "error");
        return {};
    }

    log("Fetching tasks from server...", "debug");

    std::string full_url = "https://" + server_host + ":" + std::to_string(server_port);
    httplib::Client cli(full_url);
    cli.enable_server_certificate_verification(false);
    cli.set_connection_timeout(10, 0);
    cli.set_read_timeout(10, 0);
    
    json request = {
        {"UID", uid},
        {"descr", "web-agent"},
        {"access_code", access_code}
    };
    
    std::string url = base_path + "/wa_task/";
    log("URL: " + url, "debug");
    log("Request body: " + request.dump(), "debug");
    
    auto res = cli.Post(url.c_str(), request.dump(), "application/json");

    if (!res) {
        log("Server not responding", "error");
        return {};
    }

    log("Response status: " + std::to_string(res->status), "debug");
    log("Response body: " + res->body, "debug");

    if (res->status != 200) {
        log("HTTP error: " + std::to_string(res->status), "error");
        return {};
    }

    if (res->body.empty()) {
        log("Empty response from server", "warning");
        return {};
    }

    try {
        json response = json::parse(res->body);
        std::string code = response.value("code_responce", "");
        std::string status = response.value("status", "");
        
        log("Server response: code=" + code + ", status=" + status, "debug");
        
        if (code == "0") {
            log("No tasks available (WAIT)", "debug");
            return {};
        } else if (code == "1") {
            log("Task received", "info");
            
            std::string task_code = response.value("task_code", "");
            std::string session_id = response.value("session_id", "");
            std::string options = response.value("options", "");
            
            json task = {
                {"task_id", task_code},
                {"type", task_code},
                {"session_id", session_id},
                {"options", options}
            };
            return {task};
        } else if (code == "-2") {
            log("Invalid access code", "error");
            return {};
        } else {
            log("Server error code: " + code, "error");
            return {};
        }
    } catch (const std::exception& e) {
        log("Invalid JSON from server: " + std::string(e.what()), "error");
        return {};
    }
}

bool Client::sendResultsToServer(const json& result) {
    log("Sending result to server", "debug");

    std::string full_url = "https://" + server_host + ":" + std::to_string(server_port);
    httplib::Client cli(full_url);
    cli.enable_server_certificate_verification(false);
    cli.set_connection_timeout(10, 0);
    cli.set_read_timeout(10, 0);
    
    std::string session_id = result.value("session_id", "");
    int exit_code = result.value("exit_code", 0);
    std::string message = result.value("msg", result.value("status", "completed"));
    
    json request = {
        {"UID", uid},
        {"access_code", access_code},
        {"session_id", session_id},
        {"result_code", std::to_string(exit_code)},
        {"message", message},
        {"files", "0"}
    };
    
    std::string url = base_path + "/wa_result/";
    log("URL: " + url, "debug");
    log("Request body: " + request.dump(), "debug");
    
    auto res = cli.Post(url.c_str(), request.dump(), "application/json");

    if (!res) {
        log("HTTP failed (no response)", "error");
        return false;
    } else if (res->status != 200) {
        log("HTTP error: " + std::to_string(res->status), "error");
        return false;
    }

    log("Result sent successfully", "info");
    return true;
}

void WebAgent::log(const std::string& msg, const std::string& level) {
    static const std::vector<std::string> levels = {"debug", "info", "warning", "error"};

    auto lvl = [&](const std::string& l) -> int {
        for (size_t i = 0; i < levels.size(); i++) {
            if (levels[i] == l) return static_cast<int>(i);
        }
        return -1;
    };

    if (lvl(level) < lvl(log_level)) return;

    std::filesystem::create_directories("logs");

    std::time_t now = std::time(nullptr);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%F %T", std::localtime(&now));

    std::string line = "[" + std::string(buf) + "] [" + level + "] " + msg;

    std::ofstream f("logs/agent.log", std::ios::app);
    if (f.is_open()) {
        f << line << "\n";
        f.close();
    }

    std::cout << line << std::endl;
}

WebAgent::WebAgent(const std::string& config_path) : client(config_path) {
    if (!std::filesystem::exists(config_path)) {
        throw std::runtime_error("Config file not found: " + config_path);
    }

    auto agent_handler = [](void* user, const char* section, const char* name, const char* value) -> int {
        if (!user) return 0;
       
        if (std::string(section) == "agent") {
            WebAgent* agent = static_cast<WebAgent*>(user);
            agent->setConfig(name, value);
        }
        return 1;
    };

    int result = ini_parse(config_path.c_str(), agent_handler, this);
    if (result != 0) {
        throw std::runtime_error("WebAgent config parse failed");
    }

    if (uid.empty() || server_host.empty()) {
        throw std::runtime_error("Invalid config: missing uid or server_host");
    }

    if (tasks_dir.empty()) tasks_dir = "tasks";
    if (results_dir.empty()) results_dir = "results";

    std::error_code ec;
    std::filesystem::create_directories(tasks_dir, ec);
    std::filesystem::create_directories(results_dir, ec);
    
    log("WebAgent initialized with UID: " + uid, "info");
}

void WebAgent::setConfig(const std::string& key, const std::string& value) {
    if (value.empty()) return;

    if (key == "uid") uid = value;
    else if (key == "server_host") server_host = value;
    else if (key == "server_port") server_port = std::stoi(value);
    else if (key == "base_path") base_path = value;
    else if (key == "access_code") access_code = value;
    else if (key == "tasks_dir") tasks_dir = value;
    else if (key == "results_dir") results_dir = value;
    else if (key == "task_interval") {
        try {
            task_interval = std::stoi(value);
        } catch (...) {
            log("Invalid task_interval: " + value, "error");
        }
    }
    else if (key == "log_level") log_level = value;
}

void WebAgent::checkTasks() {
    log("Checking server tasks...", "debug");

    if (!client.isValid()) {
        log("Client is not valid — skipping task check", "error");
        return;
    }

    auto server_tasks = client.fetchServerTasks();
    for (auto& task : server_tasks) {
        executeTask(task);
    }
}

void WebAgent::executeTask(const json& task) {
    if (task.is_null() || task.empty()) {
        log("Invalid task: null or empty", "error");
        return;
    }

    std::string session_id = task.value("session_id", "");
    std::string task_code = task.value("task_code", task.value("task_id", "unknown"));
    std::string task_type = task.value("type", "");

    json result = {
        {"uid", uid},
        {"task_id", task_code},
        {"session_id", session_id},
        {"exit_code", 0}
    };

    try {
        if (task_type == "CONF" || task_type == "RUN") {
            std::string options = task.value("options", "");
            if (!options.empty()) {
                log("Executing command: " + options, "info");
                int exit_code = std::system(options.c_str());
                result["exit_code"] = exit_code;
                result["status"] = (exit_code == 0) ? "completed" : "error";
            } else {
                result["status"] = "completed";
                result["msg"] = "No command to execute";
            }
        } else if (task_type == "copy_file") {
            if (!task.contains("source") || !task.contains("destination")) {
                result["status"] = "error";
                result["msg"] = "missing source/destination";
                client.sendResultsToServer(result);
                return;
            }

            std::string source = task["source"];
            std::string dest = results_dir + "/" + task["destination"].get<std::string>();

            if (!std::filesystem::exists(source)) {
                result["status"] = "error";
                result["msg"] = "source file not found";
                client.sendResultsToServer(result);
                return;
            }

            try {
                std::filesystem::copy_file(source, dest,
                    std::filesystem::copy_options::overwrite_existing);
                result["status"] = "done";
                result["file"] = dest;
            } catch (const std::filesystem::filesystem_error& e) {
                result["status"] = "error";
                result["msg"] = e.what();
            }
        } else {
            result["status"] = "completed";
            result["msg"] = "Task type: " + task_type;
        }
    } catch (const std::exception& e) {
        result["status"] = "error";
        result["msg"] = e.what();
    }

    client.sendResultsToServer(result);
}

void WebAgent::run() {
    log("Starting agent loop...", "info");

    std::this_thread::sleep_for(std::chrono::seconds(5));

    while (true) {
        checkTasks();
        std::this_thread::sleep_for(std::chrono::seconds(task_interval));
    }
}

int main() {
    try {
        WebAgent agent("../config/agent.conf");
        agent.run();
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}