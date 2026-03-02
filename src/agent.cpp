#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <regex>
#include <atomic>
#include <csignal>
#include <map>
#include <curl/curl.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define popen _popen
#define pclose _pclose
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

namespace fs = std::filesystem;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

class Logger {
private:
    std::ofstream log_file;
    std::string log_path;
    
public:
    Logger(const std::string& path) : log_path(path) {
        fs::create_directories(fs::path(path).parent_path());
        log_file.open(path, std::ios::app);
    }
    
    ~Logger() {
        if (log_file.is_open()) {
            log_file.close();
        }
    }
    
    void log(const std::string& message) {
        auto now = std::chrono::system_clock::now();
        auto now_time = std::chrono::system_clock::to_time_t(now);
        
        std::string timestamp = std::ctime(&now_time);
        timestamp.pop_back();
        
        std::string log_entry = "[" + timestamp + "] " + message;
        
        std::cout << log_entry << std::endl;
        
        if (log_file.is_open()) {
            log_file << log_entry << std::endl;
            log_file.flush();
        }
    }
};

class Config {
private:
    std::map<std::string, std::string> values;
    
public:
    bool load(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                
                values[key] = value;
            }
        }
        
        return true;
    }
    
    std::string get(const std::string& key, const std::string& default_value = "") {
        auto it = values.find(key);
        if (it != values.end()) {
            return it->second;
        }
        return default_value;
    }
    
    int getInt(const std::string& key, int default_value = 0) {
        std::string val = get(key);
        if (val.empty()) return default_value;
        return std::stoi(val);
    }
};

class CommandExecutor {
public:
    static std::string execute(const std::string& command, int timeout_seconds, Logger& logger) {
        logger.log("Выполняю команду: " + command);
        
        std::string result;
        
#ifdef _WIN32
        FILE* pipe = _popen(command.c_str(), "r");
#else
        FILE* pipe = popen(command.c_str(), "r");
#endif
        
        if (!pipe) {
            logger.log("ОШИБКА: не удалось запустить команду");
            return "ERROR: Failed to execute command";
        }
        
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        
#ifdef _WIN32
        int status = _pclose(pipe);
#else
        int status = pclose(pipe);
#endif
        
        if (status != 0) {
            logger.log("Команда завершилась с кодом: " + std::to_string(status));
        } else {
            logger.log("Команда выполнена успешно");
        }
        
        return result;
    }
};

class HttpClient {
private:
    std::string base_url;
    Logger& logger;
    
public:
    HttpClient(Logger& log) : base_url(""), logger(log) {
        curl_global_init(CURL_GLOBAL_ALL);
    }
    
    HttpClient(const std::string& url, Logger& log) : base_url(url), logger(log) {
        curl_global_init(CURL_GLOBAL_ALL);
    }
    
    ~HttpClient() {
        curl_global_cleanup();
    }
    
    void setBaseUrl(const std::string& url) {
        base_url = url;
    }
    
    bool ping() {
        std::string url = base_url + "/ping";
        std::string response;
        
        CURL* curl = curl_easy_init();
        if (!curl) return false;
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        
        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        
        return (res == CURLE_OK);
    }
    
    std::string post(const std::string& endpoint, const std::map<std::string, std::string>& params) {
        std::string url = base_url + endpoint;
        std::string response;
        std::string post_data;
        
        for (const auto& p : params) {
            if (!post_data.empty()) post_data += "&";
            
            char* escaped_key = curl_easy_escape(nullptr, p.first.c_str(), p.first.length());
            char* escaped_value = curl_easy_escape(nullptr, p.second.c_str(), p.second.length());
            
            post_data += std::string(escaped_key) + "=" + std::string(escaped_value);
            
            curl_free(escaped_key);
            curl_free(escaped_value);
        }
        
        CURL* curl = curl_easy_init();
        if (!curl) return "";
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        
        CURLcode res = curl_easy_perform(curl);
        
        if (res != CURLE_OK) {
            logger.log("CURL ошибка: " + std::string(curl_easy_strerror(res)));
        }
        
        curl_easy_cleanup(curl);
        return response;
    }
    
    bool uploadFile(const std::string& endpoint, const std::string& filepath, 
                    const std::map<std::string, std::string>& fields) {
        std::string url = base_url + endpoint;
        
        CURL* curl = curl_easy_init();
        if (!curl) return false;
        
        curl_mime* mime = curl_mime_init(curl);
        
        curl_mimepart* part = curl_mime_addpart(mime);
        curl_mime_name(part, "file");
        curl_mime_filedata(part, filepath.c_str());
        
        for (const auto& f : fields) {
            part = curl_mime_addpart(mime);
            curl_mime_name(part, f.first.c_str());
            curl_mime_data(part, f.second.c_str(), CURL_ZERO_TERMINATED);
        }
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
        
        CURLcode res = curl_easy_perform(curl);
        
        curl_mime_free(mime);
        curl_easy_cleanup(curl);
        
        return (res == CURLE_OK);
    }
};

class WebAgent {
private:
    Config config;
    Logger logger;
    HttpClient http;
    std::string uid;
    std::string server_url;
    int poll_interval;
    std::string tasks_dir;
    std::string results_dir;
    std::string current_session;
    std::atomic<bool> running;
    
public:
    WebAgent(const std::string& config_file) 
        : logger("./logs/agent.log"), 
          http(logger),
          running(true) {
        
        if (!config.load(config_file)) {
            logger.log("ОШИБКА: не удалось загрузить конфигурацию");
            throw std::runtime_error("Config load failed");
        }
        
        uid = config.get("uid", "UNKNOWN");
        server_url = config.get("server_url", "");
        poll_interval = config.getInt("poll_interval", 10);
        tasks_dir = config.get("tasks_dir", "./tasks");
        results_dir = config.get("results_dir", "./results");
        
        http.setBaseUrl(server_url);
        
        fs::create_directories(tasks_dir);
        fs::create_directories(results_dir);
        fs::create_directories("./logs");
        
        logger.log("Веб-агент инициализирован. UID: " + uid);
        logger.log("Сервер: " + server_url);
    }
    
    void stop() {
        running = false;
    }
    
    void run() {
        logger.log("Запуск веб-агента");
        
        int fail_count = 0;
        
        while (running) {
            try {
                if (!http.ping()) {
                    fail_count++;
                    int wait_time = poll_interval * (1 + fail_count);
                    logger.log("Сервер недоступен. Жду " + std::to_string(wait_time) + " сек");
                    std::this_thread::sleep_for(std::chrono::seconds(wait_time));
                    continue;
                }
                
                if (!registerAgent()) {
                    logger.log("Ошибка регистрации на сервере");
                    std::this_thread::sleep_for(std::chrono::seconds(poll_interval));
                    continue;
                }
                
                fail_count = 0;
                
                Task task = requestTask();
                
                if (task.type == "none") {
                    logger.log("Нет заданий. Жду...");
                } else {
                    executeTask(task);
                    sendResults(task);
                }
                
            } catch (const std::exception& e) {
                logger.log("Исключение: " + std::string(e.what()));
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(poll_interval));
        }
        
        logger.log("Веб-агент остановлен");
    }
    
private:
    struct Task {
        std::string type;
        std::string command;
        std::string session_id;
        std::string data;
        std::string task_id;
    };
    
    bool registerAgent() {
        std::map<std::string, std::string> params;
        params["uid"] = uid;
        params["action"] = "register";
        
        std::string response = http.post("/agent", params);
        
        if (!response.empty()) {
            std::regex session_regex("session:([^\\s]+)");
            std::smatch match;
            
            if (std::regex_search(response, match, session_regex)) {
                current_session = match[1];
                logger.log("Зарегистрирован. Сессия: " + current_session);
                return true;
            }
        }
        
        logger.log("Ошибка регистрации");
        return false;
    }
    
    Task requestTask() {
        Task task;
        task.type = "none";
        
        std::map<std::string, std::string> params;
        params["uid"] = uid;
        params["session"] = current_session;
        params["action"] = "get_task";
        
        std::string response = http.post("/agent", params);
        
        if (!response.empty()) {
            std::regex type_regex("type:([^\\s]+)");
            std::regex cmd_regex("command:(.+)");
            std::regex session_regex("session:([^\\s]+)");
            std::regex taskid_regex("task_id:([^\\s]+)");
            
            std::smatch match;
            
            if (std::regex_search(response, match, type_regex)) {
                task.type = match[1];
            }
            
            if (std::regex_search(response, match, cmd_regex)) {
                task.command = match[1];
            }
            
            if (std::regex_search(response, match, session_regex)) {
                task.session_id = match[1];
            }
            
            if (std::regex_search(response, match, taskid_regex)) {
                task.task_id = match[1];
            }
            
            logger.log("Получено задание: type=" + task.type);
        }
        
        return task;
    }
    
    void executeTask(const Task& task) {
        logger.log("Выполняю задание: " + task.type);
        
        if (task.type == "command") {
            int timeout = config.getInt("task_timeout", 300);
            std::string output = CommandExecutor::execute(task.command, timeout, logger);
            
            std::string result_file = results_dir + "/result_" + task.task_id + ".txt";
            std::ofstream out(result_file);
            out << output;
            out.close();
            
            logger.log("Результат сохранен в " + result_file);
        }
    }
    
    void sendResults(const Task& task) {
        std::vector<std::string> result_files;
        for (const auto& entry : fs::directory_iterator(results_dir)) {
            if (entry.path().filename().string().find("result_" + task.task_id) != std::string::npos) {
                result_files.push_back(entry.path().string());
            }
        }
        
        if (result_files.empty()) {
            logger.log("Нет файлов для отправки");
            return;
        }
        
        for (const auto& file : result_files) {
            std::map<std::string, std::string> fields;
            fields["session"] = task.session_id;
            fields["uid"] = uid;
            fields["task_id"] = task.task_id;
            
            if (http.uploadFile("/upload", file, fields)) {
                logger.log("Файл " + fs::path(file).filename().string() + " отправлен");
                fs::remove(file);
            } else {
                logger.log("Ошибка отправки файла");
            }
        }
    }
};

std::unique_ptr<WebAgent> g_agent;

void signalHandler(int signum) {
    std::cout << "Получен сигнал " << signum << ", останавливаю агента..." << std::endl;
    if (g_agent) {
        g_agent->stop();
    }
}

int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "   Веб-агент v1.0 (libcurl)" << std::endl;
    std::cout << "========================================" << std::endl;
    
    std::string config_file = "./config/agent.conf";
    if (argc > 1) {
        config_file = argv[1];
    }
    
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    try {
        WebAgent agent(config_file);
        g_agent = std::unique_ptr<WebAgent>(&agent);
        
        agent.run();
        
    } catch (const std::exception& e) {
        std::cerr << "Критическая ошибка: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}