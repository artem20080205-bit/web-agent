#include <cassert>
#include <fstream>
#include <iostream>
#include <filesystem>

// Эмуляция твоего WebAgent (для теста)
class TestConfig {
public:
    std::string uid;
    std::string server_uri;
    int task_interval = 30;
    
    bool loadFromFile(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) return false;
        
        std::string line;
        while (std::getline(file, line)) {
            if (line.find("uid =") != std::string::npos) {
                uid = line.substr(line.find("=") + 2);
            } else if (line.find("server_uri =") != std::string::npos) {
                server_uri = line.substr(line.find("=") + 2);
            } else if (line.find("task_interval =") != std::string::npos) {
                task_interval = std::stoi(line.substr(line.find("=") + 2));
            }
        }
        return true;
    }
};

int main() {
    // Создаём тестовый конфиг
    std::ofstream file("test_config.ini");
    file << "[agent]\n";
    file << "uid = agent-001\n";
    file << "server_uri = http://localhost:8080\n";
    file << "task_interval = 60\n";
    file.close();
    
    // Тестируем загрузку
    TestConfig config;
    bool loaded = config.loadFromFile("test_config.ini");
    
    assert(loaded);
    assert(config.uid == "agent-001");
    assert(config.server_uri == "http://localhost:8080");
    assert(config.task_interval == 60);
    
    std::cout << "Config test OK" << std::endl;
    return 0;
}