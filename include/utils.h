#pragma once

#include <iostream>
#include <fstream>
#include <string>

std::string* readFile(const char* path) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Dosya açılamadı." << std::endl;
        return NULL;
    }
    std::string* fileContents = new std::string();
    std::string line;
    while (std::getline(file, line)) {
        *fileContents += line + "\n";
    }
    return fileContents;
}