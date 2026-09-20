#include "compiler_internal.h"

void emitUint32(std::vector<uint8_t>& bytecode, uint32_t value) {
    bytecode.push_back(value & 0xFF);
    bytecode.push_back((value >> 8) & 0xFF);
    bytecode.push_back((value >> 16) & 0xFF);
    bytecode.push_back((value >> 24) & 0xFF);
}

void patchUint32(std::vector<uint8_t>& bytecode, int location, uint32_t value) {
    bytecode[location] = value & 0xFF;
    bytecode[location + 1] = (value >> 8) & 0xFF;
    bytecode[location + 2] = (value >> 16) & 0xFF;
    bytecode[location + 3] = (value >> 24) & 0xFF;
}

bool prescanRoutines(const std::vector<std::string>& lines, CompilerData* compilerData, const std::string& fileName) {
    int routineIndex = static_cast<int>(compilerData->routineList.size());
    int lineNo = 0;
    for (const auto& line : lines) {
        lineNo++;
        auto tokens = tokenizeFormula(line);
        if (tokens.empty() || (tokens[0] != "routine" && tokens[0] != "function")) continue;
        if (tokens.size() < 2) continue; // malformed; let the main pass report the error

        const std::string& name = tokens[1];

        bool returnable = tokens[0] == "function";

        int commas = 0;
        bool inParens = false;
        bool sawAnyArgToken = false;
        for (size_t i = 2; i < tokens.size(); i++) {
            const std::string& t = tokens[i];
            if (t == "(") { inParens = true; continue; }
            if (t == ")") { inParens = false; continue; }
            if (inParens) {
                sawAnyArgToken = true;
                if (t == ",") commas++;
            }
        }
        int argCount = sawAnyArgToken ? commas + 1 : 0;

        if (compilerData->routineList.count(name)) {
            printError("Routine '" + name + "' is already defined", lineNo, fileName);
            return false;
        }

        compilerData->routineList[name] = RoutineSignature{routineIndex, argCount, returnable};
        routineIndex++;
    }
    return true;
}

void printError(std::string error, int line, std::string file) {
    std::cerr << "In file " << file << std::endl;
    std::cerr << "  - Error on line " << line << std::endl << "    >>> " << error << std::endl;
}

void pushToStack(std::string token, CompilerData* data, std::vector<uint8_t>& bytecode) {
    bytecode.push_back(0x03); // PUSH opcode

    if (token.starts_with("'")) {
        auto strIndex = resolveString(token.substr(1, token.size() > 1 ? token.size() - 2 : 0), data);
        bytecode.push_back(0x01); // operand type: string
        bytecode.push_back(static_cast<uint8_t>(strIndex));
    }
    else if (isPureNumber(token)) {
        bool isFloat = isFloatLiteral(token);
        TypeTag tag = isFloat ? TAG_FLOAT : TAG_INT;
        uint8_t dataType = isFloat ? 0x05 : 0x02;
        double x = std::stod(token);
        int idx = resolveConst(x, tag, data);
        bytecode.push_back(dataType);
        bytecode.push_back(static_cast<uint8_t>(idx));
    }
    else {
        bool ref = token.starts_with("&");
        auto index = resolveVariableIndex(token, data);
        bytecode.push_back(ref ? 0x04 : 0x03);
        bytecode.push_back(static_cast<uint8_t>(index));
    }
}
