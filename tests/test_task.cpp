#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>

int main() {
    std::ofstream src("source.txt");
    src << "data";
    src.close();
    
    std::filesystem::copy_file("source.txt", "dest.txt");
    
    assert(std::filesystem::exists("dest.txt"));
    
    std::filesystem::remove("source.txt");
    std::filesystem::remove("dest.txt");
    
    std::cout << "OK" << std::endl;
    return 0;
}