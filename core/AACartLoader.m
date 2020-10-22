#import "AACartLoader.h"

#include <fstream>
#include <iostream>
#include <filesystem>
namespace fs = std::filesystem;

void cartLoader_run()
{
    fs::create_directories("sandbox/a/b");
    std::ofstream("sandbox/file1.txt");
    std::ofstream("sandbox/file2.txt");
    for(auto& p: fs::directory_iterator("sandbox"))
        std::cout << p << '\n';
    // fs::remove_all("sandbox");
}