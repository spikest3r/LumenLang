#include "disassembler.h"

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

    // Variables Section
    if (std::getline(file, line) && line.rfind("variables", 0) == 0) {
        while (std::getline(file, line)) {
            if (line == "routines") break;
            if (line.empty()) continue;

            std::string name;
            int index;
            std::stringstream ss(line);
            if (ss >> name >> index) {
                variables[index] = name;
            }
        }
    }

    // Routines Section
    while (std::getline(file, line)) {
        if (line == "exec") break;
        if (line.empty()) continue;

        std::string name = line;
        std::string offsetLine, lengthLine;

        if (!std::getline(file, offsetLine) || !std::getline(file, lengthLine))
            break;

        RoutineInfo info;
        info.offset = std::stoul(offsetLine);
        info.length = std::stoul(lengthLine);
        routines[name] = info;
    }

    // Exec Functions Section
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::string name;
        int index;

        std::stringstream ss(line);
        if (ss >> name >> index) {
            funcList[index] = name;
        }
    }

    std::cout << "Debug info loaded from " << fileName << std::endl;
    return true;
}

std::unordered_map<uint32_t, std::string> buildRoutineStarts(
    const std::unordered_map<std::string, RoutineInfo>& routines)
{
    std::unordered_map<uint32_t, std::string> routineStarts;
    for (const auto& [name, info] : routines) {
        routineStarts[info.offset] = name;
    }
    return routineStarts;
}

void disassemble(std::vector<uint8_t> bytecode,
    std::vector<std::string> stringPool,
    std::vector<double> constPool,
    std::string debugFile,
    bool* debugSymbolsLoaded,
    int vmPC)
{
    int PC = 0;
    int size = bytecode.size();

    std::unordered_map<int, std::string> variables_debug;
    std::unordered_map<std::string, RoutineInfo> routines_debug;
    std::unordered_map<int, std::string> funcList_debug;
    std::unordered_map<uint32_t, std::string> routineStarts;
    bool hasDebugData = false;

    if (!debugFile.empty()) {
        std::cout << "Loading debug info from " << debugFile << std::endl;
        bool success = loadDebugInfo(debugFile, variables_debug, routines_debug, funcList_debug);
        if (success) {
            routineStarts = buildRoutineStarts(routines_debug);
            hasDebugData = true;
            if (debugSymbolsLoaded) *debugSymbolsLoaded = true;
        }
        else {
            if (debugSymbolsLoaded) *debugSymbolsLoaded = false;
        }
    }

    bool pointMode = (vmPC != -1);

    std::cout << "===== Main =====" << std::endl;

    while (PC < size) {
        if (hasDebugData) {
            auto it = routineStarts.find(PC);
            if (it != routineStarts.end()) {
                std::cout << "\n===== Routine " << it->second << " =====\n";
            }
        }

        uint8_t byte = bytecode[PC];
        int offset = getOpCodeOffset(byte);

        if (offset <= 0) {
            offset = 1;
        }

        if (PC + offset > size) {
            std::cout << "Error: Truncated bytecode instruction at offset 0x"
                << std::hex << PC << std::dec << std::endl;
            break;
        }

        if (pointMode) {
            std::cout << (vmPC == PC ? "PC -> " : "      ");
        }

        std::cout << "0x" << std::hex << std::right << std::setw(8) << std::setfill('0') << PC << ": ";

        std::stringstream hex_stream;
        for (int i = PC; i < PC + offset; i++) {
            hex_stream << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(bytecode[i]) << " ";
        }
        std::cout << std::setfill(' ') << std::left << std::setw(15) << hex_stream.str() << " | ";

        std::string op = "UNKNOWN";
        if (disassemblyMap.count(byte)) {
            op = disassemblyMap[byte];
        }
        std::cout << std::dec << op;

        switch (byte) {
        case 0x01: // CALL (16-bit / 8-bit Target)
            if (offset > 1) {
                uint8_t targetIndex = bytecode[PC + 1];
                if (hasDebugData && routineStarts.count(targetIndex)) {
                    std::cout << " " << routineStarts[targetIndex] << " (0x"
                        << std::hex << static_cast<int>(targetIndex) << std::dec << ")";
                }
                else {
                    std::cout << " " << static_cast<int>(targetIndex);
                }
            }
            break;

        case 0x02: // POP / STORE
            if (offset > 1) {
                uint8_t varIdx = bytecode[PC + 1];
                if (hasDebugData && variables_debug.count(varIdx)) {
                    std::cout << " " << variables_debug[varIdx];
                }
                else {
                    std::cout << " var[" << static_cast<int>(varIdx) << "]";
                }
            }
            break;

        case 0x03: // PUSH <Type> <Value/Index>
            if (offset >= 3) {
                uint8_t pushType = bytecode[PC + 1];
                uint8_t operand = bytecode[PC + 2];

                if (pushType == 0x01) { // String Literal
                    if (operand < stringPool.size()) {
                        std::cout << " '" << stringPool[operand] << "'";
                    }
                    else {
                        std::cout << " str[" << static_cast<int>(operand) << "]";
                    }
                }
                else if (pushType == 0x02 || pushType == 0x05) { // Numeric / Constant Pool
                    if (operand < constPool.size()) {
                        std::cout << " " << constPool[operand];
                    }
                    else {
                        std::cout << " const[" << static_cast<int>(operand) << "]";
                    }
                }
                else if (pushType == 0x03) { // Variable Value
                    if (hasDebugData && variables_debug.count(operand)) {
                        std::cout << " " << variables_debug[operand];
                    }
                    else {
                        std::cout << " var[" << static_cast<int>(operand) << "]";
                    }
                }
                else { // Raw Immediate Integer
                    std::cout << " " << static_cast<int>(operand);
                }
            }
            break;

        case 0x04: // EXEC (Built-in standard library function)
            if (offset > 1) {
                uint8_t funcIdx = bytecode[PC + 1];
                if (hasDebugData && funcList_debug.count(funcIdx)) {
                    std::cout << " " << funcList_debug[funcIdx];
                }
                else {
                    std::cout << " fn[" << static_cast<int>(funcIdx) << "]";
                }
            }
            break;

            // 8-bit Jumps / Branches
        case 0x05: // JUMP
        case 0xB0: // JEQ
        case 0xB1: // JGR
        case 0xB2: // JLS
        case 0xB3: // JGE
        case 0xB4: // JLE
        case 0xB5: // JNE
            if (offset > 1) {
                uint32_t targetAddr = bytecode[PC + 1];
                std::cout << " 0x" << std::hex << std::right
                    << std::setw(8) << std::setfill('0') << targetAddr;
            }
            break;

            // 32-bit Jumps, Calls, and Branches
        case 0x06: // JUMP32
        case 0x07: // CALL32
        case 0xC0: // JEQ32
        case 0xC1: // JGR32
        case 0xC2: // JLS32
        case 0xC3: // JGE32
        case 0xC4: // JLE32
        case 0xC5: // JNE32
            if (offset == 5) {
                uint32_t targetAddr = bytecode[PC + 1] |
                    (bytecode[PC + 2] << 8) |
                    (bytecode[PC + 3] << 16) |
                    (bytecode[PC + 4] << 24);
                std::cout << " 0x" << std::hex << std::right
                    << std::setw(8) << std::setfill('0') << targetAddr;
            }
            break;

        default:
            // Fallback for generic instructions
            for (int i = PC + 1; i < PC + offset; i++) {
                std::cout << " " << static_cast<int>(bytecode[i]);
            }
            break;
        }

        std::cout << std::dec << std::endl;
        PC += offset; // Guaranteed to increment PC!
    }

    std::cout << std::dec << std::endl;
}