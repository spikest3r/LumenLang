#pragma once
#include "includes.h"
#include "helpers.h"
#include "types.h"
#include "tokenizer.h"

int compile(std::string fileName, 
    std::vector<int>& bytecode,
    std::unordered_map<std::string, int>& variableMap,
    std::vector<std::string>& stringPool, std::unordered_map<std::string, int>& stringPoolMap,
    int& variableIndex, int& stringIndex, bool verbose = false
);

void compileExpression(std::string expr, 
    std::vector<int>& bytecode, 
    std::unordered_map<std::string, int>& variableMap, 
    int& variableIndex
);
