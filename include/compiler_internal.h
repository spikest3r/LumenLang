#pragma once

#include "compiler.h"

enum class BlockType {
    IF,
    WHILE,
    REPEAT
};

struct LoopJumpData {
    int locStart;
    std::vector<int> unresolvedEnds;
    std::string iteratorVarName;
};

struct SubroutineInfo {
    std::vector<std::string> args;
};

struct UnresolvedJump {
    std::string keyword;
    int location;
    int line;
    int routineIndex; // -1 for main program, >= 0 for subroutines
    int argCount = 0; // only for routine calls
};

// State shared across the compile loop and the finalize/link stage of a single
// compileFromStream() call. Bundled here so the two stages can live in
// separate translation units.
struct CompileState {
    std::unordered_map<std::string, int> globalLabels;
    std::unordered_map<int, std::unordered_map<std::string, int>> routineLabels;

    std::unordered_map<int, std::vector<uint8_t>> subroutineBytecode;
    std::unordered_map<std::string, int> subroutineIndexMap;
    std::unordered_map<int, SubroutineInfo> subroutineInfoMap;
    std::vector<UnresolvedJump> unresolvedRoutineCalls;
    std::vector<UnresolvedJump> unresolvedJumps;
    std::vector<int> condJumpStack;
    std::vector<std::vector<int>> elseJumpStack;

    std::string functionArgument = "";

    std::vector<BlockType> blockDepth;
    std::vector<bool> elseDefined;

    std::vector<LoopJumpData> loopCondJumpStack;

    std::string conditionTokens;

    int lineIndex = 1;
    bool inRoutine = false;
    bool inFunction = false;
    int routineIndex = -1;
    int routineCount = 0;

    int funcArgs = 0;
    int requiredFuncArgs = 0;

    int loopDepth = 0;
};

extern std::unordered_map<std::string, Function> funcList;
extern const std::unordered_map<std::string, ConditionOp> condOpMap;
extern const std::unordered_map<ConditionOp, uint8_t> condOpcodeMap;

void emitUint32(std::vector<uint8_t>& bytecode, uint32_t value);
void patchUint32(std::vector<uint8_t>& bytecode, int location, uint32_t value);
void printError(std::string error, int line);
void pushToStack(std::string token, CompilerData* data, std::vector<uint8_t>& bytecode);
void prescanRoutines(const std::vector<std::string>& lines, CompilerData* compilerData);

// Compiles a single already-tokenized source line into bytecode, updating
// CompileState in place. Returns -1 on error (after calling printError), 0 on success.
int compileLine(const std::string& line, CompileState& state, CompilerData* compilerData,
    bool verbose, bool debugInfo);

// Resolves jumps/calls, merges subroutine bytecode into the main bytecode, and
// optionally writes the debug info file. Returns -1 on error, 0 on success.
int finalizeCompile(CompileState& state, CompilerData* compilerData,
    bool debugInfo, const std::string& fileName);
