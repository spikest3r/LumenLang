#pragma once
#include "includes.h"
#include "helpers.h"
#include "types.h"

using NativeFn = std::function<void(std::vector<Variant>&, std::vector<Variant>&)>;

int64_t getInt(const Variant& v);

extern std::unordered_map<int, NativeFn> funcMap;

int run(
    const std::vector<int>& bytecode,
    const std::vector<std::string>& stringPool,
    const int& variableIndex
);

int run_debug(
    const std::string& filename,
    const std::vector<int>& bytecode,
    const std::vector<std::string>& stringPool,
    const int& variableIndex
);

int execute(
    const std::vector<int>& bytecode,
    const std::vector<std::string>& stringPool,
    std::vector<Variant>& variables,
    std::vector<Variant>& stack,
    std::vector<int>& pcStack,
    const int& PC,
    bool& halt
);