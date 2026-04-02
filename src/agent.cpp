#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <filesystem>
#include <ctime>

#include "httplib.h"
#include <nlohmann/json.hpp>
#include "ini.h"

using json = nlohmann::json;

// Клиент

class Client {
private:
    std::string uid;
    std::string server_uri;
    std::string access_code;
    std::string log_level = "info";

public:
    Client(const std::string &config_path);
    void setConfig(const std::string& key, const std::string& value);
    bool isValid() const;
    std::vector<json> fetchServerTasks();
    bool sendResultsToServer(const json& result);
    void log(const std::string& msg, const std::string& level = "info");
};

// Webagent

class WebAgent {
private:
    std::string uid;
    std::string server_uri;
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
    std::vector<json> readLocalTasks();
    void executeTask(const json& task);
    void checkTasks();
};



// handler

auto handler = [](void* user, const char* section, const char* name, const char* value) -> int {
    if (!user) {
        std::cerr << "Handler: user is null" << std::endl;
        return 0;
    }

    // Определяем, какой объект передан, по наличию метода setConfig
    // Но лучше передавать оба объекта отдельно
   
    if (std::string(section) == "agent") {
        WebAgent* agent = static_cast<WebAgent*>(user);
        agent->setConfig(name, value);
    } else if (std::string(section) == "client") {
        // Здесь user может указывать на WebAgent, а не на Client!
        // Нужно передавать отдельный указатель для client
    }
    return 1;
}; 






// CONF client

Client::Client(const std::string &config_path) {
    if (!std::filesystem::exists(config_path)) {
        throw std::runtime_error("Client config file not found: " + config_path);
    }

    // Создаем отдельный handler для Client
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
}


void Client::setConfig(const std::string& key, const std::string& value) {
    if (value.empty()) return;

    if (key == "uid") uid = value;
    else if (key == "server_uri") server_uri = value;
    else if (key == "access_code") access_code = value;
    else if (key == "log_level") log_level = value;
}

bool Client::isValid() const {
    return !uid.empty() && !server_uri.empty() && !access_code.empty();
}


// CONF Webagent

WebAgent::WebAgent(const std::string& config_path) : client(config_path) {
    if (!std::filesystem::exists(config_path)) {
        throw std::runtime_error("Config file not found: " + config_path);
    }

    // Создаем отдельный handler для WebAgent
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

    // Проверка обязательных полей
    if (uid.empty() || server_uri.empty()) {
        throw std::runtime_error("Invalid config: missing uid or server_uri");
    }

    // Инициализация директорий
    if (tasks_dir.empty()) tasks_dir = "tasks";
    if (results_dir.empty()) results_dir = "results";

    std::error_code ec;
    std::filesystem::create_directories(tasks_dir, ec);
    if (ec) {
        log("Failed to create tasks directory: " + ec.message(), "error");
    }

    std::filesystem::create_directories(results_dir, ec);
    if (ec) {
        log("Failed to create results directory: " + ec.message(), "error");
    }
}





void WebAgent::setConfig(const std::string& key, const std::string& value) {
    if (value.empty()) return; // Пропускаем пустые значения

    if (key == "uid") uid = value;
    else if (key == "server_uri") server_uri = value;
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


// fetchServerTasks

std::vector<json> Client::fetchServerTasks() {
    if (server_uri.empty()) {
        log("Server URI is empty", "error");
        return {};
    }

    log("Fetching tasks from server...", "debug");

    httplib::Client cli(server_uri);
    auto res = cli.Get(("/tasks?uid=" + uid).c_str());

    if (!res) {
        log("Server not responding", "error");
        return {};
    }

    if (res->status != 200) {
        log("HTTP error: " + std::to_string(res->status), "error");
        return {};
    }

    if (res->body.empty()) {
        log("Empty response from server", "warning");
        return {};
    }

    try {
        auto arr = json::parse(res->body);
        log("Received " + std::to_string(arr.size()) + " tasks from server", "info");
        return arr;
    } catch (...) {
        log("Invalid JSON from server", "error");
        return {};
    }
}




// sendResultsToServer

bool Client::sendResultsToServer(const json& result) {
    log("Sending result to server: " + result.dump(), "debug");

    httplib::Client cli(server_uri);

    httplib::Headers headers = {
        {"Authorization", access_code}
    };

    auto res = cli.Post("/results", headers, result.dump(), "application/json");

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





// log


void Client::log(const std::string& msg, const std::string& level) {
    static const std::vector<std::string> levels = {"debug", "info", "warning", "error"};

    auto lvl = [&](const std::string& l) -> int {
        for (size_t i = 0; i < levels.size(); i++) {
            if (levels[i] == l) return static_cast<int>(i);
        }
        return -1; // уровень не найден
    };

    if (lvl(level) < lvl(log_level)) return;

    std::error_code ec;
    std::filesystem::create_directories("logs", ec);
    if (ec) {
        std::cerr << "Failed to create logs directory: " << ec.message() << std::endl;
        return;
    }

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


void WebAgent::log(const std::string& msg, const std::string& level) {
    static const std::vector<std::string> levels = {"debug", "info", "warning", "error"};

    auto lvl = [&](const std::string& l) -> int {
        for (size_t i = 0; i < levels.size(); i++) {
            if (levels[i] == l) return static_cast<int>(i);
        }
        return -1; // уровень не найден
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


// проверить задачу на наличие

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


// выполнить задачу

void WebAgent::executeTask(const json& task) {
    if (task.is_null() || task.empty()) {
        log("Invalid task: null or empty", "error");
        return;
    }

    json result = {
        {"uid", uid},
        {"task_id", task.value("task_id", "unknown")}
    };

    try {
        if (!task.contains("type")) {
            log("Task missing 'type' field", "error");
            result["status"] = "error";
            result["msg"] = "missing type field";
            client.sendResultsToServer(result);
            return;
        }
        std::string type = task["type"];

        if (type == "copy_file") {
            if (!task.contains("source") || !task.contains("destination")) {
                log("Task missing source or destination", "error");
                result["status"] = "error";
                result["msg"] = "missing source/destination";
                client.sendResultsToServer(result);
                return;
            }

            std::string source = task["source"];
            std::string dest = task["destination"];

            if (!std::filesystem::exists(source)) {
                log("Source file not found: " + source, "error");
                result["status"] = "error";
                result["msg"] = "source file not found";
                client.sendResultsToServer(result);
                return;
            }

            try {
                std::filesystem::copy_file(
                    source, dest,
            std::filesystem::copy_options::overwrite_existing
                );
                result["status"] = "done";
            } catch (const std::filesystem::filesystem_error& e) {
                log("File copy failed: " + std::string(e.what()), "error");
                result["status"] = "error";
                result["error"] = e.what();
            }
        } else {
            result["status"] = "error";
            result["msg"] = "unknown task";
        }
    } catch (const std::exception& e) {
        result["status"] = "error";
        result["error"] = e.what();
    }

    client.sendResultsToServer(result);
}



// run
void WebAgent::run() {
    log("Starting agent loop...", "info");

    // Пауза перед первой проверкой
    std::this_thread::sleep_for(std::chrono::seconds(5));

    while (true) {
        checkTasks();
        std::this_thread::sleep_for(std::chrono::seconds(task_interval));
    }
}






int main() {
    try {
        WebAgent agent("config/agent.conf");
        agent.run();
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
