#include "compiler.h"
#include "vm.h"
#include "disassembler.h"
#include "programfile.h"

int stringIndex = 0;
int variableIndex = 0;

int main(int argc, char** argv) {
    if(argc < 2) {
        std::cout << "Usage: interpreter (file) [options]" << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  --verbose      Enable verbose output" << std::endl;
        std::cout << "  --compile      Compile the source file (default)" << std::endl;
        std::cout << "  --run          Run the compiled bytecode (default)" << std::endl;
        std::cout << "  --disassemble  Disassemble the compiled bytecode" << std::endl;
        std::cout << "  --dbgsym       Generate debug symbols information file" << std::endl;
        std::cout << "  --debugger     Run file in debugger mode" << std::endl;
        std::cout << "  --pico         Compile with Rapsberry Pi Pico functions" << std::endl;
        std::cout << "If no options are provided, the program will compile and run the source file." << std::endl;
        return -1;
    }

    std::string file_name = argv[1];
    bool verboseFlag = false;
    bool compileFlag = false;
    bool runFlag = false;
    bool disassembleFlag = false;
    bool debugInfo = false;
    bool debuggerActive = false;
    bool picoFlag = false;

    if(argc > 2) {
        for(int i = 2; i < argc; i++) {
            auto arg = argv[i];
            if(strcmp(arg, "--verbose") == 0) verboseFlag = true;
            else if(strcmp(arg, "--compile") == 0) compileFlag = true;
            else if(strcmp(arg, "--disassemble") == 0) disassembleFlag = true;
            else if(strcmp(arg, "--run") == 0) runFlag = true;
            else if(strcmp(arg, "--debug") == 0) debugInfo = true;
            else if(strcmp(arg, "--pico") == 0) picoFlag = true;
            else if(strcmp(arg, "--dbgsym") == 0) debugInfo = true;
            else if(strcmp(arg, "--debugger") == 0) debuggerActive = true;
            else {
                std::cerr << "Unknown argument: " << arg << std::endl;
                return -1;
            }
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

    if(
        (debuggerActive && compileFlag && !runFlag) ||
        (debugInfo && !compileFlag)
    ) {
        std::cerr << "Invalid flag combination" << std::endl;
        return -1;
    }

    if(disassembleFlag) {
        BinaryProgram inProg;
        inProg.load(file_name);
        disassemble(inProg.bytecode, inProg.stringPool, file_name + ".dbg");
    }

    if(compileFlag) {
        std::vector<int> bytecode;
        std::vector<std::string> stringPool;

        std::unordered_map<std::string, int> variableMap;
        std::unordered_map<std::string, int> stringPoolMap;

        int status = compile(
            file_name, 
            bytecode, variableMap, stringPool, stringPoolMap, 
            variableIndex, stringIndex, verboseFlag, debugInfo, picoFlag
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

        BinaryProgram inProg;
        inProg.load(inFile);
        
        int status = -1;

        if(debuggerActive) {
            status = run_debug(
                file_name,
                inProg.bytecode,
                inProg.stringPool,
                inProg.variableIndex
            );
        } else {
            if(verboseFlag) std::cout << "Executing..." << std::endl;

            status = run(
                inProg.bytecode,
                inProg.stringPool,
                inProg.variableIndex
            );

            if(verboseFlag) std::cout << "Execution finished" << std::endl;
        }
    }

    return 0;
}