#include <cassert>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>

void executeTask(const nlohmann::json& task, std::string& out_status) {
    if (!task.contains("type")) {
        out_status = "error: missing type";
        return;
    }
    
    std::string type = task["type"];
    
    if (type == "copy_file") {
        std::string source = task["source"];
        std::string dest = task["destination"];
        
        if (!std::filesystem::exists(source)) {
            out_status = "error: source not found";
            return;
        }
        
        try {
            std::filesystem::copy_file(source, dest, 
                std::filesystem::copy_options::overwrite_existing);
            out_status = "done";
        } catch (...) {
            out_status = "error: copy failed";
        }
    } else {
        out_status = "error: unknown type";
    }
}

int main() {
    std::cout << "\n=== REAL WebAgent Test ===\n" << std::endl;
    
    int passed = 0;
    int failed = 0;
    
    std::cout << "Test 1: copy existing file... ";
    std::ofstream src("real_source.txt");
    src << "test data";
    src.close();
    
    nlohmann::json task = {
        {"type", "copy_file"},
        {"source", "real_source.txt"},
        {"destination", "real_dest.txt"}
    };
    
    std::string status;
    executeTask(task, status);
    
    if (status == "done" && std::filesystem::exists("real_dest.txt")) {
        std::cout << "OK" << std::endl;
        passed++;
    } else {
        std::cout << "FAIL (status=" << status << ")" << std::endl;
        failed++;
    }
    
    std::cout << "Test 2: copy nonexistent file... ";
    task = {
        {"type", "copy_file"},
        {"source", "nonexistent.txt"},
        {"destination", "nowhere.txt"}
    };
    
    executeTask(task, status);
    
    if (status.find("error") != std::string::npos) {
        std::cout << "OK" << std::endl;
        passed++;
    } else {
        std::cout << "FAIL" << std::endl;
        failed++;
    }
    
    std::cout << "Test 3: unknown task type... ";
    task = {{"type", "unknown_type"}};
    
    executeTask(task, status);
    
    if (status == "error: unknown type") {
        std::cout << "OK" << std::endl;
        passed++;
    } else {
        std::cout << "FAIL" << std::endl;
        failed++;
    }
    
    std::cout << "Test 4: missing type field... ";
    task = {{"source", "a.txt"}};
    
    executeTask(task, status);
    
    if (status == "error: missing type") {
        std::cout << "OK" << std::endl;
        passed++;
    } else {
        std::cout << "FAIL" << std::endl;
        failed++;
    }
    
    std::filesystem::remove("real_source.txt");
    std::filesystem::remove("real_dest.txt");
    
    std::cout << "\n=== Results ===" << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;
    
    return 0;
}