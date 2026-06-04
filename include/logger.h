#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <iostream>
#include <ctime>
#include <sstream>
#include <iomanip>

enum class LogLevel { DEBUG, INFO, WARNING, ERROR_LVL };

class Logger {
public:
    static Logger& instance() {
        static Logger inst;
        return inst;
    }

    void init(const std::string& filepath, LogLevel level = LogLevel::INFO) {
        std::lock_guard<std::mutex> lock(mutex_);
        level_ = level;
        if (!filepath.empty()) {
            file_.open(filepath, std::ios::app);
        }
    }

    void log(LogLevel level, const std::string& msg) {
        if (level < level_) return;
        std::lock_guard<std::mutex> lock(mutex_);
        std::string line = format(level, msg);
        std::cout << line << std::endl;
        if (file_.is_open()) {
            file_ << line << std::endl;
            file_.flush();
        }
    }

    void debug(const std::string& m)   { log(LogLevel::DEBUG,   m); }
    void info(const std::string& m)    { log(LogLevel::INFO,    m); }
    void warning(const std::string& m) { log(LogLevel::WARNING, m); }
    void error(const std::string& m)   { log(LogLevel::ERROR_LVL, m); }

private:
    Logger() = default;
    std::mutex    mutex_;
    std::ofstream file_;
    LogLevel      level_{LogLevel::INFO};

    std::string format(LogLevel level, const std::string& msg) {
        auto now = std::time(nullptr);
        std::ostringstream oss;
        oss << "[" << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S") << "] ";
        switch (level) {
            case LogLevel::DEBUG:     oss << "[DEBUG]   "; break;
            case LogLevel::INFO:      oss << "[INFO]    "; break;
            case LogLevel::WARNING:   oss << "[WARNING] "; break;
            case LogLevel::ERROR_LVL: oss << "[ERROR]   "; break;
        }
        oss << msg;
        return oss.str();
    }
};

#define LOG_DEBUG(m)   Logger::instance().debug(m)
#define LOG_INFO(m)    Logger::instance().info(m)
#define LOG_WARN(m)    Logger::instance().warning(m)
#define LOG_ERROR(m)   Logger::instance().error(m)
