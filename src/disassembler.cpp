#include "disassembler.h"

enum OperandType {
    NONE = 0,
    TYPE = 1,
    VALUE = 2
};

struct RoutineInfo
{
    uint32_t offset;
    uint32_t length;
};

std::map<int, std::string> disassemblyMap = {
    {0x01, "CALL"},
    {0x02, "POP"},
    {0x03, "PUSH"},
    {0x04, "EXEC"},
    {0x05, "JUMP"},
    
    {0xA0, "ADD"},
    {0xA1, "SUB"},
    {0xA2, "MUL"},
    {0xA3, "DIV"},
    {0xA4, "POW"},
    {0xA5, "MOD"},
    
    {0xB0, "JEQ"},
    {0xB1, "JGR"},
    {0xB2, "JLS"},
    {0xB3, "JGE"},
    {0xB4, "JLE"},
    {0xB5, "JNE"},

    {0xAA, "JOIN"},

    {0xFE, "RET"},
    {0xFF, "HLT"}
};

bool loadDebugInfo(const std::string& fileName,
    std::unordered_map<int, std::string>& variables,
    std::unordered_map<std::string, RoutineInfo>& routines,
    std::unordered_map<int, std::string>& funcList
) {
    std::ifstream file(fileName);

    if (!file) {
        std::cerr << "Error: Could not open debug file " << fileName << std::endl;
        return false;
    }

    std::string line;

    // Variables
    std::getline(file, line); // "variables"

    while (std::getline(file, line))
    {
        if (line == "routines")
            break;

        std::string name;
        int index;

        std::stringstream ss(line);
        ss >> name >> index;

        variables[index] = name;
    }

    // Routines
    while (std::getline(file, line))
    {
        if (line == "exec")
            break;

        std::string name = line;

        std::string offsetLine;
        std::string lengthLine;

        if (!std::getline(file, offsetLine))
            break;
        if (!std::getline(file, lengthLine))
            break;

        RoutineInfo info;
        info.offset = std::stoul(offsetLine);
        info.length = std::stoul(lengthLine);

        routines[name] = info;
    }

    // Exec functions
    while (std::getline(file, line))
    {
        std::string name;
        int index;

        std::stringstream ss(line);
        ss >> name >> index;

        funcList[index] = name;
    }

    std::cout << "Debug info loaded from " << fileName << std::endl;

    return true;
}

std::unordered_map<uint32_t, std::string> buildRoutineStarts(
    const std::unordered_map<std::string, RoutineInfo>& routines)
{
    std::unordered_map<uint32_t, std::string> routineStarts;

    for (const auto& [name, info] : routines)
    {
        routineStarts[info.offset] = name;
    }

    return routineStarts;
}

void disassemble(std::vector<int> bytecode, std::vector<std::string> stringPool, std::string debugFile) {
    int PC = 0;
    int size = bytecode.size();

    std::unordered_map<int, std::string> variables_debug;
    std::unordered_map<std::string, RoutineInfo> routines_debug;
    std::unordered_map<int, std::string> funcList_debug;
    std::unordered_map<uint32_t, std::string> routineStarts;
    bool hasDebugData = false;

    if(debugFile != "") {
        std::cout << "Loading debug info from " << debugFile << std::endl;
        loadDebugInfo(debugFile, variables_debug, routines_debug, funcList_debug);
        routineStarts = buildRoutineStarts(routines_debug);
        hasDebugData = true;
    }

    std::cout << "===== Main =====" << std::endl;
    
    while(PC < size) {
        auto offset = getOpCodeOffset(bytecode[PC]);

        std::cout << "0x" << std::hex << std::right << std::setw(8) << std::setfill('0') << PC << ": ";

        std::stringstream hex_stream;
        for(int i = PC; i < PC + offset; i++) {
            hex_stream << std::hex << std::setw(2) << std::setfill('0')
                    << static_cast<int>(bytecode[i]) << " ";
        }

        std::cout << std::setfill(' ') << std::left << std::setw(15) << hex_stream.str() << " | ";

        std::cout << std::dec;

        auto byte = bytecode[PC];
        auto op = disassemblyMap[byte];
        std::cout << op;

        int j = 0;
        for(int i = PC + 1; i < PC + offset; i++) {
            if(!op.empty() && op[0] == 'J') {
                std::cout << " 0x" 
                        << std::hex << std::right << std::setw(8) << std::setfill('0')
                        << static_cast<int>(bytecode[i]);
                
                std::cout << std::dec << std::setfill(' ') << std::left; 
            } else {
                switch(byte) {
                    case 0x01: // CALL
                        {
                            auto rt = routineStarts[bytecode[PC + 1] - 1];
                            std::cout << " " << rt << " (0x" 
                                    << std::hex << std::right << std::setw(8) << std::setfill('0')
                                    << static_cast<int>(bytecode[i]) << ")";
                        }
                        break;
                    case 0x02: // POP
                        std::cout << " " << variables_debug[bytecode[PC + 1]];
                        break;
                    case 0x03: // PUSH
                        if(j < 1) {
                            if(bytecode[PC + 1] == 0x01) {
                                std::cout << " '" << stringPool[bytecode[PC + 2]] << "'";
                            } else if(bytecode[PC + 1] == 0x02) {
                                std::cout << " " << std::dec << static_cast<int>(bytecode[PC + 2]);
                            } else if(bytecode[PC + 1] == 0x03) {
                                std::cout << " " << variables_debug[bytecode[PC + 2]];
                            }
                        }
                        break;
                    case 0x04: // EXEC
                        {
                            std::cout << " " << funcList_debug[bytecode[PC + 1]];
                        }
                        break;
                    default:
                        std::cout << " " << std::dec << static_cast<int>(bytecode[i]);
                        break;
                }
            }
            j++;
        }

        std::cout << std::endl;

        if(hasDebugData) {
            auto it = routineStarts.find(PC);
            if (it != routineStarts.end())
            {
                std::cout << "\n===== Routine " << it->second << " =====\n";
            }
        }
        
        PC += offset;
    }

    std::cout << std::dec << std::endl;
}