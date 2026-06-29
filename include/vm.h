#pragma once
#include "includes.h"
#include "helpers.h"
#include "types.h"

int execute(
    const std::vector<int>& bytecode,
    const std::vector<std::string>& stringPool,
    const int& variableIndex
);