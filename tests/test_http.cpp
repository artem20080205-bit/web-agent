#include <cassert>
#include <iostream>

int main() {
    std::string url = "http://xdev.arkcom.ru:9999";
    assert(!url.empty());
    
    std::cout << "OK" << std::endl;
    return 0;
}