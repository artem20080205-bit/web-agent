#include <cassert>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main() {
    std::string json_str = R"({"task_id": "123", "type": "copy_file"})";
    json task = json::parse(json_str);
    
    assert(task["task_id"] == "123");
    assert(task["type"] == "copy_file");
    
    std::cout << "OK" << std::endl;
    return 0;
}