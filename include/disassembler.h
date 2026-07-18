#pragma once
#include "includes.h"
#include "helpers.h"

struct RoutineInfo
{
    uint32_t offset;
    uint32_t length;
};

void disassemble(std::vector<uint8_t> bytecode, 
    std::vector<std::string> stringPool, 
    std::vector<int> constPool,
    std::string debugFile = "", 
    bool* debugSymbolsLoaded = nullptr,
    int vmPC = -1
);

bool loadDebugInfo(const std::string& fileName,
    std::unordered_map<int, std::string>& variables,
    std::unordered_map<std::string, RoutineInfo>& routines,
    std::unordered_map<int, std::string>& funcList
);