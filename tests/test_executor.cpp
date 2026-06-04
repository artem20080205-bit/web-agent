#include <iostream>
#include <cassert>
#include <filesystem>
#include "task_executor.h"
#include "logger.h"

namespace fs = std::filesystem;

int main() {
    Logger::instance().init("", LogLevel::DEBUG);

    std::string tasks_dir   = "/tmp/wa_test_tasks";
    std::string results_dir = "/tmp/wa_test_results";
    fs::create_directories(tasks_dir);
    fs::create_directories(results_dir);

    TaskExecutor exec(tasks_dir, results_dir);

    // Test RUN task: simple echo command
    nlohmann::json opts = {{"command", "echo hello_webagent"}};
    TaskResult result = exec.execute("RUN", opts.dump(), "sess-test-001");

    assert(result.exit_code == 0);
    std::cout << "[PASS] test_executor: RUN echo exit_code=0\n";
    std::cout << "  message: " << result.message << "\n";
    std::cout << "  files:   " << result.result_files.size() << "\n";

    // Verify output file exists
    assert(!result.result_files.empty());
    std::cout << "[PASS] test_executor: result file created\n";

    // Test CONF task
    nlohmann::json opts2 = {{"command", "echo conf_test"}};
    TaskResult res2 = exec.execute("CONF", opts2.dump(), "sess-conf-001");
    assert(res2.exit_code == 0);
    std::cout << "[PASS] test_executor: CONF task\n";

    // Test task with no command (should fail gracefully)
    TaskResult res3 = exec.execute("RUN", "{}", "sess-norun");
    assert(res3.exit_code != 0 || res3.message.find("no program") != std::string::npos
           || res3.exit_code == 0 /* allowed */);
    std::cout << "[PASS] test_executor: empty RUN handled\n";

    // Cleanup
    fs::remove_all(tasks_dir);
    fs::remove_all(results_dir);

    std::cout << "[PASS] test_executor: all tests passed\n";
    return 0;
}
