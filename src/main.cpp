#include "programfile.h"
#include "vm.h"
#include "compiler.h"
#include "disassembler.h"
#include <emscripten/bind.h>

static BinaryProgram program;

bool loadProgram(const std::string& filename) {
    return program.load(filename);
}

bool saveProgram(const std::string& filename) {
    return program.save(filename);
}

int runVM() {
    if (program.bytecode.empty()) {
        std::cerr << "bytecode empty, did load succeed?\n";
        return -1;
    }
    return run(program.bytecode, program.stringPool, program.variableIndex);
}

int compileScript(const std::string& script) {
    // TODO: Pass flags
    return compile(
        script,
        program.bytecode,
        program.stringPool,
        program.variableIndex,
        false,
        false
    );
}

void init() {
    program.bytecode = {};
    program.stringPool = {};
    program.variableIndex = 0;
}

std::string disassembleBin() {
    return disassemble(
        program.bytecode,
        program.stringPool
    );
}

EMSCRIPTEN_BINDINGS(module) {
    emscripten::function("loadProgram", &loadProgram);
    emscripten::function("saveProgram", &saveProgram);
    emscripten::function("runVM", &runVM);
    emscripten::function("compileScript", &compileScript);
    emscripten::function("init", &init);
    emscripten::function("disassembleBin", &disassembleBin);
}