#include "vm.h"

int64_t getInt(const Variant& v) {
    return std::get<int64_t>(v.data);
}

double getNumeric(const Variant& v) {
    if (v.type == TAG_FLOAT) return std::get<double>(v.data);
    return static_cast<double>(std::get<int64_t>(v.data));
}

bool isFloatVariant(const Variant& a, const Variant& b) {
    return a.type == TAG_FLOAT || b.type == TAG_FLOAT;
}

int run(
    VMProgramData* progData
) {
    VMExecutionData execData;

    execData.variables.resize(progData->variableCount);

    while(true) {
        auto opcode = progData->bytecode[execData.PC];
        int offset = getOpCodeOffset(opcode);
        
        int result = execute(progData, &execData);

        if(execData.halt) break;
        execData.PC = result;
    }
    return 0;
}

int execute(
    VMProgramData* progData,
    VMExecutionData* execData
) {
    auto opcode = progData->bytecode[execData->PC];
    int offset = getOpCodeOffset(opcode);

    switch(opcode) {
        case 0x01:
            // run subroutine
            {
                auto addr = progData->bytecode[execData->PC + 1];
                execData->pcStack.push_back(execData->PC + offset);
                return addr;
            }
            break;
        case 0xFE:
            // return from subroutine
            {
                if(execData->pcStack.empty()) {
                    std::cerr << "Return stack underflow!" << std::endl;
                    return -1;
                }
                int newPC = execData->pcStack.back();
                execData->pcStack.pop_back();
                return newPC;
            }
            break;
        case 0x02:
            {
                auto varIndex = progData->bytecode[execData->PC + 1];
                auto var = execData->stack.back();
                execData->variables[varIndex].type = var.type;
                switch(var.type) {
                    case TAG_INT:
                        execData->variables[varIndex].data = std::get<int64_t>(var.data);
                        break;
                    case TAG_FLOAT:
                        execData->variables[varIndex].data = std::get<double>(var.data);
                        break;
                    case TAG_STRING:
                        execData->variables[varIndex].data = std::get<std::string>(var.data);
                        break;
                }
                execData->stack.pop_back();
            }
            break;
        case 0x03:
            {
                auto dataType = progData->bytecode[execData->PC + 1];
                auto value = progData->bytecode[execData->PC + 2];
                switch(dataType) {
                    case 0x01:
                        execData->stack.push_back({TAG_STRING, progData->stringPool[value]});
                        break;
                    case 0x02:
                        execData->stack.push_back({TAG_INT, static_cast<int64_t>(progData->constPool[value])});
                        break;
                    case 0x03:
                        {
                            auto variable = execData->variables[value];
                            execData->stack.push_back({variable.type, variable.data});
                        }
                        break;
                    case 0x04:
                        {
                            // raw uint8_t passed
                            execData->stack.push_back({TAG_INT, static_cast<int64_t>(value)});
                        }
                        break;
                    case 0x05:
                        execData->stack.push_back({TAG_FLOAT, progData->constPool[value]});
                        break;
                }
            }
            break;
        case 0x04: {
            auto functionIndex = progData->bytecode[execData->PC + 1];
            auto it = funcMap.find(functionIndex);
            if (it != funcMap.end()) {
                it->second(execData->stack, execData->variables);
            } else {
                std::cerr << "Unknown function index: " << functionIndex << std::endl;
                return -1;
            }
            break;
        }
        case 0x05:
            {
                auto newPC = progData->bytecode[execData->PC + 1];
                return newPC;
            }
            break;
        case 0xA0: { // ADD
            Variant b = execData->stack.back(); execData->stack.pop_back();
            Variant a = execData->stack.back(); execData->stack.pop_back();
            Variant result;
            if (isFloatVariant(a, b)) {
                result.type = TAG_FLOAT;
                result.data = getNumeric(a) + getNumeric(b);
            } else {
                result.type = TAG_INT;
                result.data = getInt(a) + getInt(b);
            }
            execData->stack.push_back(result);
            break;
        }
        case 0xA1: { // SUB
            Variant b = execData->stack.back(); execData->stack.pop_back();
            Variant a = execData->stack.back(); execData->stack.pop_back();
            Variant result;
            if (isFloatVariant(a, b)) {
                result.type = TAG_FLOAT;
                result.data = getNumeric(a) - getNumeric(b);
            } else {
                result.type = TAG_INT;
                result.data = getInt(a) - getInt(b);
            }
            execData->stack.push_back(result);
            break;
        }
        case 0xA2: { // MUL
            Variant b = execData->stack.back(); execData->stack.pop_back();
            Variant a = execData->stack.back(); execData->stack.pop_back();
            Variant result;
            if (isFloatVariant(a, b)) {
                result.type = TAG_FLOAT;
                result.data = getNumeric(a) * getNumeric(b);
            } else {
                result.type = TAG_INT;
                result.data = getInt(a) * getInt(b);
            }
            execData->stack.push_back(result);
            break;
        }
        case 0xA3: { // DIV
            Variant b = execData->stack.back(); execData->stack.pop_back();
            Variant a = execData->stack.back(); execData->stack.pop_back();
            Variant result;
            result.type = TAG_FLOAT;
            result.data = getNumeric(a) / getNumeric(b);
            execData->stack.push_back(result);
            break;
        }
        case 0xA4: { // POW
            Variant b = execData->stack.back(); execData->stack.pop_back();
            Variant a = execData->stack.back(); execData->stack.pop_back();
            Variant result;
            if (isFloatVariant(a, b)) {
                result.type = TAG_FLOAT;
                result.data = std::pow(getNumeric(a), getNumeric(b));
            } else {
                result.type = TAG_INT;
                result.data = static_cast<int64_t>(std::pow(getNumeric(a), getNumeric(b)));
            }
            execData->stack.push_back(result);
            break;
        }
        case 0xA5: { // MOD
            Variant b = execData->stack.back(); execData->stack.pop_back();
            Variant a = execData->stack.back(); execData->stack.pop_back();
            Variant result;
            if (isFloatVariant(a, b)) {
                result.type = TAG_FLOAT;
                result.data = std::fmod(getNumeric(a), getNumeric(b));
            } else {
                result.type = TAG_INT;
                result.data = getInt(a) % getInt(b);
            }
            execData->stack.push_back(result);
            break;
        }
        case 0xB0: // ==
        case 0xB1: // >
        case 0xB2: // <
        case 0xB3: // >=
        case 0xB4: // <=
        case 0xB5: // != 
        {
            Variant b = execData->stack.back(); execData->stack.pop_back();
            Variant a = execData->stack.back(); execData->stack.pop_back();
            int falseIndex = progData->bytecode[execData->PC + 1];
            
            bool result = false;
            double av = getNumeric(a);
            double bv = getNumeric(b);
            
            switch (opcode) {
                case 0xB0: result = av == bv; break;
                case 0xB1: result = av >  bv; break;
                case 0xB2: result = av <  bv; break;
                case 0xB3: result = av >= bv; break;
                case 0xB4: result = av <= bv; break;
                case 0xB5: result = av != bv; break;
            }
            
            if (!result) {
                return falseIndex;
            }
            break;
        }
        case 0xAA: { // join strings
            int strCount = getInt(execData->stack.back()); execData->stack.pop_back();
            std::string result;
            for(int i = 0; i < strCount; i++) {
                Variant strVar = execData->stack.back(); execData->stack.pop_back();
                std::string str = std::get<std::string>(strVar.data);
                result = str + result; // prepend to maintain order
            }
            execData->stack.push_back({TAG_STRING, result});
            break;
        }
        case 0xFF:
            execData->halt = true;
            break;
        default:
            std::cout << "Invalid opcode" << std::endl;
            return -1; // error
    }

    return execData->PC + offset;
}