#include "compiler.h"
#include "vm.h"
#include "disassembler.h"
#include "programfile.h"

int stringIndex = 0;
int variableIndex = 0;

int main(int argc, char** argv) {
    if(argc < 2) {
        std::cout << "Usage: interpreter (file) [--verbose] [--compile] [--run]" << std::endl;
        return -1;
    }

    std::string file_name = argv[1];
    bool verboseFlag = false;
    bool compileFlag = false;
    bool runFlag = false;
    bool disassembleFlag = false;
    bool picoFlag = false;

    if(argc > 2) {
        for(int i = 2; i < argc; i++) {
            auto arg = argv[i];
            if(strcmp(arg, "--verbose") == 0) verboseFlag = true;
            if(strcmp(arg, "--compile") == 0) compileFlag = true;
            if(strcmp(arg, "--disassemble") == 0) disassembleFlag = true;
            if(strcmp(arg, "--run") == 0) runFlag = true;
            if(strcmp(arg, "--pico") == 0) picoFlag = true;
        }
    }

    if(!compileFlag && !runFlag && !disassembleFlag) {
        compileFlag = true;
        runFlag = true;
    }

    if(disassembleFlag && (runFlag || compileFlag)) {
        std::cerr << "Cannot compile/run in diassemble mode!" << std::endl;
        return -1;
    }

    if(disassembleFlag) {
        BinaryProgram inProg;
        inProg.load(file_name);
        disassemble(inProg.bytecode);
    }

    if(compileFlag) {
        std::vector<int> bytecode;
        std::vector<std::string> stringPool;

        std::unordered_map<std::string, int> variableMap;
        std::unordered_map<std::string, int> stringPoolMap;

        int status = compile(
            file_name, 
            bytecode, variableMap, stringPool, stringPoolMap, 
            variableIndex, stringIndex, verboseFlag, picoFlag
        );

        if(status != 0) {
            std::cerr << "Compilation failed!" << std::endl;
            return status;
        }

        if(verboseFlag) {
            std::cout << "Bytecode: ";
            for (uint8_t b : bytecode) {
                std::cout << std::hex << std::setw(2) << std::setfill('0')
                        << static_cast<int>(b) << " ";
            }
            std::cout << std::dec << std::endl;
            
        }

        BinaryProgram outProg;
        outProg.bytecode = bytecode;
        outProg.stringPool = stringPool;
        outProg.variableIndex = variableIndex;
        outProg.save(file_name + ".bin");
    }

    if(runFlag) {
        std::string inFile = compileFlag ? file_name + ".bin" : file_name;

        if(verboseFlag) std::cout << "Executing..." << std::endl;

        BinaryProgram inProg;
        inProg.load(inFile);
        
        int status = execute(
            inProg.bytecode,
            inProg.stringPool,
            inProg.variableIndex
        );

        if(verboseFlag) std::cout << "Execution finished" << std::endl;
    }

    return 0;
}