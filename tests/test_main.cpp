#include <cstdlib>
#include <iostream>

int main() {
    int passed = 0;
    int failed = 0;
    
    if (system("./test_config") == 0) passed++; else failed++;
    if (system("./test_logger") == 0) passed++; else failed++;
    if (system("./test_parser") == 0) passed++; else failed++;
    if (system("./test_task") == 0) passed++; else failed++;
    if (system("./test_http") == 0) passed++; else failed++;
    
    std::cout << "Passed: " << passed << " Failed: " << failed << std::endl;
    return 0;
}