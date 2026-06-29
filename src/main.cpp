#include "includes.h"
#include "helpers.h"
#include "compiler.h"
#include "types.h"
#include "vm.h"

std::unordered_map<std::string, int> variableMap;
std::vector<std::string> stringPool;
std::unordered_map<std::string, int> stringPoolMap;
std::unordered_map<std::string, int> funcList = {
    {"print", 0x01}
};

int stringIndex = 0;
int variableIndex = 0;

int main(int argc, char** argv) {
    if(argc < 2) {
        std::cout << "Usage: interpreter script_file [--verbose]" << std::endl;
        return -1;
    }

    auto file_name = argv[1];
    bool verbose = false;

    if(argc > 2) {
        for(int i = 2; i < argc; i++) {
            auto arg = argv[i];
            if(strcmp(arg, "--verbose") == 0) verbose = true;
        }
    }

    std::vector<int> bytecode;

    int status = compile(
        file_name, 
        bytecode, variableMap, stringPool, stringPoolMap, 
        funcList, variableIndex, stringIndex, verbose
    );

    if(status != 0) {
        // TODO: Error message
        return status;
    }

    if(verbose) {
        for (uint8_t b : bytecode) {
            std::cout << std::hex << std::setw(2) << std::setfill('0')
                    << static_cast<int>(b) << " ";
        }
        std::cout << std::dec << std::endl;
        std::cout << "Executing..." << std::endl;
    }
    
    status = execute(
        bytecode,
        stringPool,
        variableIndex
    );

    if(verbose) std::cout << "Execution finished" << std::endl;

    return 0;
}