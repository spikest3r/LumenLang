#include "compiler.h"
#include "vm.h"
#include "disassembler.h"
#include "programfile.h"
#include "examples.h"
#include "types.h"

int stringIndex = 0;
int variableIndex = 0;

int main(int argc, char** argv) {
    if(argc < 2) {
        std::cout << "No input file provided.\n";
        std::cout << "Not sure where to start? Try 'lumen --introduction'!" << std::endl;
        return -1;
    }

    std::string file_name = argv[1];

    if(file_name == "--introduction") {
        std::ofstream file("helloworld.lmn");
        file << "println 'Hello, world!'\n";
        file << "name = ''\n";
        file << "print 'What`s your name? '\n";
        file << "inputStr &name\n";
        file << "greeting = 'Hello, ' .. name .. '!'\n";
        file << "println greeting\n";
        file.close();

        std::cout << "\n👋 Welcome to LumenLang!\n";
        std::cout << "--------------------------------------------------\n";
        std::cout << "✨ A starter script 'helloworld.lmn' was created for you.\n";
        std::cout << "   This script will say hello and ask for your name.\n\n";
        std::cout << "Ready to try it out? Run this command:\n";
        std::cout << "👉  lumen helloworld.lmn\n\n";
        std::cout << "Want more examples? Try running 'lumen --examples'.\n";
        std::cout << "For docs, visit: https://lumen.olehsheremeta.com/\n";
        return 0;
    } else if(file_name == "--examples") {
        if(argc == 2) {
            std::cout << "Available examples: \n";
            std::cout << "  age               Age calculator\n";
            std::cout << "  infinite-loop     Infinite loop demonstrating labels and jumps\n";
            std::cout << "  temperature       Temperature converter\n";
            std::cout << "  fizzbuzz          Classical FizzBuzz algorithm\n\n";
            std::cout << "Generate an example:\n";
            std::cout << "  lumen --examples <name>\n\n";
            std::cout << "Example:\n";
            std::cout << "  lumen --exampless fizzbuzz\n";
            std::cout << std::endl;
            return 0;
        } else if(argc == 3) {
            auto exampleToLoad = argv[2];
            auto it = exampleMap.find(exampleToLoad);
            if(it != exampleMap.end()) {
                // write file
                auto name = it->second.fileName;
                auto code = *(it->second.codePtr);
                std::ofstream file(name);
                file << code.c_str();
                file.close();
                std::cout << "✨ Created '" << name << "'!\n";
                std::cout << "👉 Run it with: lumen " << name << "\n";
                return 0;
            } else {
                std::cerr << "Unknown example" << std::endl;
                return -1;
            }
        } else {
            std::cerr << "Invalid arguments" << std::endl;
            return -1;
        }
    } else if(file_name == "--help") {
        std::cout << "Usage: lumen (file) [options]" << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  --verbose                Enable verbose output" << std::endl;
        std::cout << "  --compile                Compile the source file (default)" << std::endl;
        std::cout << "  --run                    Run the compiled bytecode (default)" << std::endl;
        std::cout << "  --disassemble            Disassemble the compiled bytecode" << std::endl;
        std::cout << "  --dbgsym                 Generate debug symbols information file" << std::endl;
        std::cout << "  --debugger               Run file in debugger mode" << std::endl;
        std::cout << "If no options are provided, the program will compile and run the source file." << std::endl;
        return 0;
    } else if(file_name == "--version") {
        if(argc != 2) {
            std::cerr << "Invalid arguments" << std::endl;
            return -1;
        }
        std::cout
            << "LumenLang v" << LUMEN_VERSION << "\n"
            << "Branch: " << GIT_BRANCH << "\n"
            << "Commit: " << GIT_COMMIT << "\n"
            << "Build: " << BUILD_DATE << "\n";
        return 0;
    }
    
    bool verboseFlag = false;
    bool compileFlag = false;
    bool runFlag = false;
    bool disassembleFlag = false;
    bool debugInfo = false;
    bool debuggerActive = false;

    if(argc > 2) {
        for(int i = 2; i < argc; i++) {
            auto arg = argv[i];
            if(strcmp(arg, "--verbose") == 0) verboseFlag = true;
            else if(strcmp(arg, "--compile") == 0) compileFlag = true;
            else if(strcmp(arg, "--disassemble") == 0) disassembleFlag = true;
            else if(strcmp(arg, "--run") == 0) runFlag = true;
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
        disassemble(inProg.bytecode, inProg.stringPool, inProg.constPool, file_name + ".dbg");
    }

    if(compileFlag) {
        CompilerData data;

        int status = compile(
            file_name, &data, verboseFlag, debugInfo
        );

        if(status != 0) {
            std::cerr << "Compilation failed!" << std::endl;
            return status;
        }

        if(verboseFlag) {
            std::cout << "Bytecode: ";
            for (uint8_t b : data.bytecode) {
                std::cout << std::hex << std::setw(2) << std::setfill('0')
                        << static_cast<int>(b) << " ";
            }
            std::cout << std::dec << std::endl;
            
        }

        BinaryProgram outProg;
        outProg.bytecode = data.bytecode;
        outProg.stringPool = data.stringPool;
        outProg.constPool = data.constPool;
        outProg.variableCount = data.variableCount;
        outProg.save(file_name + ".bin");
    }

    if(runFlag) {
        std::string inFile = compileFlag ? file_name + ".bin" : file_name;

        BinaryProgram inProg;
        inProg.load(inFile);
        
        int status = -1;

        VMProgramData progData;
        constructProgData(&progData, &inProg);

        if(debuggerActive) {
            status = run_debug(
                &progData
            );
        } else {
            if(verboseFlag) std::cout << "Executing..." << std::endl;

            status = run(
                &progData
            );

            if(verboseFlag) std::cout << "Execution finished" << std::endl;
        }
    }

    return 0;
}