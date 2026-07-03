#pragma once
#include "includes.h"
#include "types.h"
void replaceAll(std::string& str, const std::string& from, const std::string& to);
bool isPureNumber(const std::string& s);
int resolveVariableIndex(std::string keyword, std::unordered_map<std::string, int>& variableMap, int& variableIndex);
int resolveString(std::string str, std::vector<std::string>& stringPool, std::unordered_map<std::string, int>& stringPoolMap, int& stringIndex);
int getOpCodeOffset(int opcode);
bool isVar(const std::string &s);
std::string variantToString(const Variant& v);