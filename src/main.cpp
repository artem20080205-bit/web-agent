#include <iostream>
#include <string>
#include <csignal>
#include <atomic>
#include "config.h"
#include "logger.h"
#include "agent.h"

static Agent* g_agent = nullptr;

void signal_handler(int sig) {
    std::cout << "\nSignal " << sig << " received. Shutting down...\n";
    if (g_agent) g_agent->stop();
}

void print_usage(const char* prog) {
    std::cout << "Usage: " << prog << " [--config <path>] [--uid <uid>] [--server <url>]\n\n"
              << "  --config <path>   Config JSON (default: ./config/agent.json)\n"
              << "  --uid <uid>       Override UID\n"
              << "  --server <url>    Override server URL\n"
              << "  --help            This help\n";
}

int main(int argc, char* argv[]) {
    std::string config_path = "./config/agent.json";
    std::string uid_override, server_override;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--help" || a == "-h")       { print_usage(argv[0]); return 0; }
        else if (a == "--config" && i+1<argc) config_path = argv[++i];
        else if (a == "--uid"    && i+1<argc) uid_override = argv[++i];
        else if (a == "--server" && i+1<argc) server_override = argv[++i];
    }

    Config cfg;
    try {
        cfg = Config::load(config_path);
    } catch (const std::exception& e) {
        std::cerr << "Config warning: " << e.what() << "\nUsing defaults.\n";
        cfg.uid         = "agent-001";
        cfg.descr       = "web-agent";
        cfg.server_url  = "https://xdev.arkcom.ru:9999/app/webagent1/api";
        cfg.tasks_dir   = "./tasks";
        cfg.results_dir = "./results";
        cfg.log_file    = "./webagent.log";
        cfg.ssl_verify  = false;
    }

    if (!uid_override.empty())    cfg.uid        = uid_override;
    if (!server_override.empty()) cfg.server_url = server_override;

    Logger::instance().init(cfg.log_file, LogLevel::DEBUG);
    LOG_INFO("Web-Agent v1.0 | UID: " + cfg.uid);
    LOG_INFO("Server: " + cfg.server_url);

    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Pass config_path so SET_PARAM/SET_INTERVAL can update it
    Agent agent(cfg, config_path);
    g_agent = &agent;

    try {
        agent.run();
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Fatal: ") + e.what());
        return 1;
    }

    return 0;
}
