#include "programfile.h"
#include "vm.h"
#include "compiler.h"
#include "disassembler.h"
#include "examples.h"
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
    VMProgramData progData;
    constructProgData(&progData, &program);
    return run(&progData);
}

int compileScript(const std::string& script) {
    // TODO: Pass flags
    CompilerData data;

    int status = compile(
        script,
        &data,
        false,
        false
    );

    if(status != 0) return status;

    program.bytecode = data.bytecode;
    program.stringPool = data.stringPool;
    program.constPool = data.constPool;
    program.variableCount = data.variableCount;

    return status;
}

void init() {
    program.bytecode = {};
    program.stringPool = {};
    program.variableCount = 0;
}

std::string disassembleBin() {
    return disassemble(
        program.bytecode,
        program.stringPool,
        program.constPool
    );
}

std::string getExample(const std::string& key) {
    auto it = exampleMap.find(key);
    if(it != exampleMap.end()) {
        return *(it->second.codePtr);
    } else {
        // return generic example
        std::cout << "Failed to load example: " << key << std::endl;
        return "println 'Hello, world!'";
    }
} 

static std::vector<uint8_t> lmnbin;

extern "C"
{

    uint8_t* getLmnbin() {
        lmnbin = program.saveStream();
        return lmnbin.data();
    }


    size_t getLmnbinSize()
    {
        return lmnbin.size();
    }

}

EMSCRIPTEN_BINDINGS(module) {
    emscripten::function("loadProgram", &loadProgram);
    emscripten::function("saveProgram", &saveProgram);
    emscripten::function("runVM", &runVM);
    emscripten::function("compileScript", &compileScript);
    emscripten::function("init", &init);
    emscripten::function("disassembleBin", &disassembleBin);
    emscripten::function("getExample", &getExample);
}