#include <cassert>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class TaskManager {
private:
    std::string tasks_dir = "test_tasks";
    std::string results_dir = "test_results";
    
public:
    TaskManager() {
        std::filesystem::create_directories(tasks_dir);
        std::filesystem::create_directories(results_dir);
    }
    
    void addTask(const json& task) {
        std::string task_id = task["task_id"];
        std::ofstream file(tasks_dir + "/" + task_id + ".json");
        file << task.dump();
        file.close();
    }
    
    std::vector<json> getAllTasks() {
        std::vector<json> tasks;
        for (const auto& entry : std::filesystem::directory_iterator(tasks_dir)) {
            if (entry.path().extension() == ".json") {
                std::ifstream file(entry.path());
                json task;
                file >> task;
                tasks.push_back(task);
            }
        }
        return tasks;
    }
    
    void deleteTask(const std::string& task_id) {
        std::string path = tasks_dir + "/" + task_id + ".json";
        if (std::filesystem::exists(path)) {
            std::filesystem::remove(path);
        }
    }
    
    std::string executeTask(const json& task) {
        if (!task.contains("type")) {
            return "error: missing type";
        }
        
        std::string type = task["type"];
        
        if (type == "copy_file") {
            std::string source = task["source"];
            std::string dest = results_dir + "/" + task.value("destination", "out.txt");
            
            if (!std::filesystem::exists(source)) {
                return "error: source not found";
            }
            
            try {
                std::filesystem::copy_file(source, dest,
                    std::filesystem::copy_options::overwrite_existing);
                return "done";
            } catch (...) {
                return "error: copy failed";
            }
        }
        
        return "error: unknown type";
    }
    
    void cleanup() {
        std::filesystem::remove_all(tasks_dir);
        std::filesystem::remove_all(results_dir);
    }
};

int main() {
    std::cout << "\n=== TASKS LIFECYCLE TEST ===\n" << std::endl;
    
    int passed = 0;
    int failed = 0;
    
    TaskManager tm;
    
    std::cout << "Test 1: Add task... ";
    
    std::ofstream src("test_source.txt");
    src << "test data";
    src.close();
    
    json task1 = {
        {"task_id", "task_001"},
        {"type", "copy_file"},
        {"source", "test_source.txt"},
        {"destination", "output.txt"}
    };
    
    tm.addTask(task1);
    auto tasks = tm.getAllTasks();
    
    if (tasks.size() == 1 && tasks[0]["task_id"] == "task_001") {
        std::cout << "OK" << std::endl;
        passed++;
    } else {
        std::cout << "FAIL" << std::endl;
        failed++;
    }
    
    std::cout << "Test 2: Execute task... ";
    std::string result = tm.executeTask(task1);
    
    if (result == "done" && std::filesystem::exists("test_results/output.txt")) {
        std::cout << "OK" << std::endl;
        passed++;
    } else {
        std::cout << "FAIL" << std::endl;
        failed++;
    }
    
    std::cout << "Test 3: Add multiple tasks... ";
    
    json task2 = {
        {"task_id", "task_002"},
        {"type", "copy_file"},
        {"source", "test_source.txt"},
        {"destination", "output2.txt"}
    };
    
    json task3 = {
        {"task_id", "task_003"},
        {"type", "copy_file"},
        {"source", "test_source.txt"},
        {"destination", "output3.txt"}
    };
    
    tm.addTask(task2);
    tm.addTask(task3);
    tasks = tm.getAllTasks();
    
    if (tasks.size() == 3) {
        std::cout << "OK" << std::endl;
        passed++;
    } else {
        std::cout << "FAIL" << std::endl;
        failed++;
    }
    
    std::cout << "Test 4: Delete task... ";
    tm.deleteTask("task_002");
    tasks = tm.getAllTasks();
    
    bool found = false;
    for (const auto& t : tasks) {
        if (t["task_id"] == "task_002") found = true;
    }
    
    if (!found && tasks.size() == 2) {
        std::cout << "OK" << std::endl;
        passed++;
    } else {
        std::cout << "FAIL" << std::endl;
        failed++;
    }
    
    std::cout << "Test 5: Delete nonexistent task... ";
    tm.deleteTask("task_999");
    tasks = tm.getAllTasks();
    
    if (tasks.size() == 2) {
        std::cout << "OK" << std::endl;
        passed++;
    } else {
        std::cout << "FAIL" << std::endl;
        failed++;
    }
    
    std::cout << "Test 6: Execute remaining tasks... ";
    result = tm.executeTask(task3);
    
    if (result == "done" && std::filesystem::exists("test_results/output3.txt")) {
        std::cout << "OK" << std::endl;
        passed++;
    } else {
        std::cout << "FAIL" << std::endl;
        failed++;
    }
    
    std::cout << "Test 7: Delete all tasks... ";
    tm.deleteTask("task_001");
    tm.deleteTask("task_003");
    tasks = tm.getAllTasks();
    
    if (tasks.empty()) {
        std::cout << "OK" << std::endl;
        passed++;
    } else {
        std::cout << "FAIL" << std::endl;
        failed++;
    }
    
    std::filesystem::remove("test_source.txt");
    tm.cleanup();
    
    std::cout << "\n=== RESULTS ===" << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;
    
    return 0;
}
