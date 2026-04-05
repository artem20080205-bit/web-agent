#include <cassert>
#include <fstream>
#include <filesystem>
#include <iostream>

int main() {
    std::filesystem::create_directories("logs");
    
    std::ofstream log("logs/agent.log", std::ios::app);
    log << "[2024-01-01] [INFO] Test" << std::endl;
    log.close();
    
    assert(std::filesystem::file_size("logs/agent.log") > 0);
    
    std::cout << "OK" << std::endl;
    return 0;
}