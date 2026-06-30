#pragma once
#include "includes.h"

class BinaryProgram {
public:
    std::vector<int> bytecode;
    std::vector<std::string> stringPool;
    int variableIndex = 0;
    bool save(const std::string& path);
    bool load(const std::string& path);
};