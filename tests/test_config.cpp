#include <iostream>
#include <fstream>
#include <cassert>
#include "config.h"

int main() {
    // Write a temp config
    {
        std::ofstream f("/tmp/test_agent.json");
        f << R"({
            "uid": "test-007",
            "descr": "test-agent",
            "server_url": "https://example.com/api",
            "tasks_dir": "/tmp/tasks",
            "results_dir": "/tmp/results",
            "log_file": "/tmp/test.log",
            "poll_interval_sec": 5,
            "max_poll_interval_sec": 60,
            "ssl_verify": false
        })";
    }

    Config cfg = Config::load("/tmp/test_agent.json");
    assert(cfg.uid == "test-007");
    assert(cfg.descr == "test-agent");
    assert(cfg.server_url == "https://example.com/api");
    assert(cfg.poll_interval_sec == 5);
    assert(cfg.ssl_verify == false);

    // Test missing file throws
    bool threw = false;
    try { Config::load("/nonexistent/path.json"); }
    catch (...) { threw = true; }
    assert(threw);

    std::cout << "[PASS] test_config\n";
    return 0;
}
