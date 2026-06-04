#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>

struct Config {
    std::string uid;
    std::string descr;
    std::string server_url;
    std::string tasks_dir;
    std::string results_dir;
    std::string log_file;
    int         poll_interval_sec{10};
    int         max_poll_interval_sec{120};
    bool        ssl_verify{true};

    static Config load(const std::string& path) {
        std::ifstream f(path);
        if (!f.is_open())
            throw std::runtime_error("Cannot open config: " + path);

        nlohmann::json j;
        try { f >> j; }
        catch (const std::exception& e) {
            throw std::runtime_error(std::string("Config parse error: ") + e.what());
        }

        Config c;
        c.uid                  = j.value("uid", "agent-001");
        c.descr                = j.value("descr", "web-agent");
        c.server_url           = j.value("server_url", "https://xdev.arkcom.ru:9999/app/webagent1/api");
        c.tasks_dir            = j.value("tasks_dir", "./tasks");
        c.results_dir          = j.value("results_dir", "./results");
        c.log_file             = j.value("log_file", "./webagent.log");
        c.poll_interval_sec    = j.value("poll_interval_sec", 10);
        c.max_poll_interval_sec = j.value("max_poll_interval_sec", 120);
        c.ssl_verify           = j.value("ssl_verify", true);
        return c;
    }
};
