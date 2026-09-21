#pragma once
#include "includes.h"
#include "helpers.h"
#include "types.h"
#include "tokenizer.h"

struct CompileState;

struct Function {
    uint8_t opcode;
    uint8_t argCount;
    bool returnable;
};

int compile(std::string fileName,
    CompilerData* compilerData,
    bool verbose = false, bool debugInfo = false
);

int compileFromFile(std::ifstream& file,
    CompilerData* compilerData,
    bool verbose = false, bool debugInfo = false, std::string fileName = "", CompileState* prevState = nullptr
);

int compileFromText(const std::string& text,
    CompilerData* compilerData,
    bool verbose = false, bool debugInfo = false, std::string fileName = ""
);

void compileExpression(
    std::string expr, CompilerData* data, std::vector<uint8_t>& bytecode
);