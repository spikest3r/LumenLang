#pragma once
#include "includes.h"
#include "types.h"

struct CompilerData {
    std::vector<uint8_t> bytecode;

    std::vector<std::string> stringPool;
    std::vector<int> constPool;
    int variableCount = 0;

    std::unordered_map<std::string, int> variableMap;
    std::unordered_map<std::string, int> stringPoolMap;
    std::unordered_map<int, int> constPoolMap;
};

void replaceAll(std::string& str, const std::string& from, const std::string& to);
bool isPureNumber(const std::string& s);
int resolveVariableIndex(std::string keyword, CompilerData* data);
int resolveString(std::string str, CompilerData* data);
int resolveConst(int constValue, CompilerData* data);
int getOpCodeOffset(int opcode);
bool isVar(const std::string &s);
std::string variantToString(const Variant& v);