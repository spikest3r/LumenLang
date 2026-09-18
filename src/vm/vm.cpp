#include "vm.h"
#include "helpers.h"

inline uint32_t readU32(const std::vector<uint8_t>& bytecode, size_t& pc)
{
    uint32_t value =
        (uint32_t)bytecode[pc] |
        ((uint32_t)bytecode[pc + 1] << 8) |
        ((uint32_t)bytecode[pc + 2] << 16) |
        ((uint32_t)bytecode[pc + 3] << 24);

    pc += 4;
    return value;
}

int64_t getInt(const Variant& v) {
    return std::get<int64_t>(v.data);
}

double getNumeric(const Variant& v) {
    if (v.type == TAG_FLOAT) return std::get<double>(v.data);
    if (v.type == TAG_INT) return static_cast<double>(std::get<int64_t>(v.data));
    return 0.0;
}

bool isFloatVariant(const Variant& a, const Variant& b) {
    return a.type == TAG_FLOAT || b.type == TAG_FLOAT;
}

int run(
    VMProgramData* progData
) {
    VMExecutionData execData;
    execData.PC = 0;
    execData.routineBase = 0;

    while(true) {
        auto opcode = progData->bytecode[execData.PC];
        int offset = getOpCodeOffset(opcode);

        int result = execute(progData, &execData);

        if(execData.halt || result == -1)
            break;
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

    switch (opcode) {
    case 0x01:
    {
        auto addr = progData->bytecode[execData->PC + 1];
        execData->pcStack.push_back({
            execData->PC + offset,
            execData->routineBase
        });

        execData->routineBase = addr;

        return addr;
    }
    break;
    case 0xFE:
    {
        if (execData->pcStack.empty()) {
            std::cerr << "Return stack underflow!" << std::endl;
            return -1;
        }

        auto frame = execData->pcStack.back();
        execData->pcStack.pop_back();

        execData->routineBase = frame.routineBase;

        return frame.returnPC;
    }
    break;
    case 0x02:
    {
        auto varIndex = progData->bytecode[execData->PC + 1];
        Variant var = execData->stack.back();

        writeVariable(execData, varIndex, var);

        execData->stack.pop_back();
    }
    break;
    case 0x03:
    {
        auto dataType = progData->bytecode[execData->PC + 1];
        auto value = progData->bytecode[execData->PC + 2];
        switch (dataType) {
        case 0x01:
            execData->stack.push_back({ TAG_STRING, progData->stringPool[value] });
            break;
        case 0x02:
            execData->stack.push_back({ TAG_INT, static_cast<int64_t>(progData->constPool[value]) });
            break;
        case 0x03:
        {
            Variant variable = readVariable(execData, value);
            execData->stack.push_back(variable);
        }
        break;
        case 0x04:
        {
            execData->stack.push_back({ TAG_INT, static_cast<int64_t>(value) });
        }
        break;
        case 0x05:
            execData->stack.push_back({ TAG_FLOAT, progData->constPool[value] });
            break;
        }
    }
    break;
    case 0x04: {
        auto functionIndex = progData->bytecode[execData->PC + 1];
        auto it = funcMap.find(functionIndex);
        if (it != funcMap.end()) {
            try {
                it->second(execData);
            } catch(std::exception& s) {
                std::cerr << "Function error\n" << s.what() << std::endl;
                execData->halt = true;
            }
        }
        else {
            std::cerr << "Unknown function index: " << functionIndex << std::endl;
            return -1;
        }
        break;
    }
    case 0x05:
    {
        auto newPC = progData->bytecode[execData->PC + 1];
        return execData->routineBase + newPC;
    }
    break;
    case 0x06:
    {
        size_t pc = execData->PC + 1;
        uint32_t addr = readU32(progData->bytecode, pc);

        return execData->routineBase + addr;
    }
    break;
    case 0x07:
    {
        size_t pc = execData->PC + 1;
        uint32_t target = readU32(progData->bytecode, pc);

        execData->pcStack.push_back({
            execData->PC + offset,
            execData->routineBase
            });

        execData->routineBase = target;

        return target;
    }
    break;
    case 0xA0: {
        if(execData->stack.size() < 2) {
            execData->stack.push_back({TAG_INT, 0});
            break;
        }
        Variant b = execData->stack.back(); execData->stack.pop_back();
        Variant a = execData->stack.back(); execData->stack.pop_back();
        Variant result;
        if (isFloatVariant(a, b)) {
            result.type = TAG_FLOAT;
            result.data = getNumeric(a) + getNumeric(b);
        }
        else {
            result.type = TAG_INT;
            result.data = getInt(a) + getInt(b);
        }
        execData->stack.push_back(result);
        break;
    }
    case 0xA1: {
        if(execData->stack.size() < 2) {
            execData->stack.push_back({TAG_INT, 0});
            break;
        }
        Variant b = execData->stack.back(); execData->stack.pop_back();
        Variant a = execData->stack.back(); execData->stack.pop_back();
        Variant result;
        if (isFloatVariant(a, b)) {
            result.type = TAG_FLOAT;
            result.data = getNumeric(a) - getNumeric(b);
        }
        else {
            result.type = TAG_INT;
            result.data = getInt(a) - getInt(b);
        }
        execData->stack.push_back(result);
        break;
    }
    case 0xA2: {
        if(execData->stack.size() < 2) {
            execData->stack.push_back({TAG_INT, 0});
            break;
        }
        Variant b = execData->stack.back(); execData->stack.pop_back();
        Variant a = execData->stack.back(); execData->stack.pop_back();
        Variant result;
        if (isFloatVariant(a, b)) {
            result.type = TAG_FLOAT;
            result.data = getNumeric(a) * getNumeric(b);
        }
        else {
            result.type = TAG_INT;
            result.data = getInt(a) * getInt(b);
        }
        execData->stack.push_back(result);
        break;
    }
    case 0xA3: {
        if(execData->stack.size() < 2) {
            execData->stack.push_back({TAG_INT, 0});
            break;
        }
        Variant b = execData->stack.back(); execData->stack.pop_back();
        Variant a = execData->stack.back(); execData->stack.pop_back();
        Variant result;
        result.type = TAG_FLOAT;
        result.data = getNumeric(a) / getNumeric(b);
        execData->stack.push_back(result);
        break;
    }
    case 0xA4: {
        if(execData->stack.size() < 2) {
            execData->stack.push_back({TAG_INT, 0});
            break;
        }
        Variant b = execData->stack.back(); execData->stack.pop_back();
        Variant a = execData->stack.back(); execData->stack.pop_back();
        Variant result;
        if (isFloatVariant(a, b)) {
            result.type = TAG_FLOAT;
            result.data = std::pow(getNumeric(a), getNumeric(b));
        }
        else {
            result.type = TAG_INT;
            result.data = static_cast<int64_t>(std::pow(getNumeric(a), getNumeric(b)));
        }
        execData->stack.push_back(result);
        break;
    }
    case 0xA5: {
        if(execData->stack.size() < 2) {
            execData->stack.push_back({TAG_INT, 0});
            break;
        }
        Variant b = execData->stack.back(); execData->stack.pop_back();
        Variant a = execData->stack.back(); execData->stack.pop_back();
        Variant result;
        if (isFloatVariant(a, b)) {
            result.type = TAG_FLOAT;
            result.data = std::fmod(getNumeric(a), getNumeric(b));
        }
        else {
            result.type = TAG_INT;
            result.data = getInt(a) % getInt(b);
        }
        execData->stack.push_back(result);
        break;
    }
    case 0xA6: {
        if (!execData->stack.empty()) {
            auto* x = &execData->stack.back();

            switch (x->type) {
            case TAG_FLOAT:
                std::get<double>(x->data)++;
                break;

            case TAG_INT:
                std::get<int64_t>(x->data)++;
                break;

            case TAG_STRING:
                break;
            }
        }
        break;
    }
    case 0xA7: {
        if (!execData->stack.empty()) {
            auto* x = &execData->stack.back();

            switch (x->type) {
            case TAG_FLOAT:
                std::get<double>(x->data)--;
                break;

            case TAG_INT:
                std::get<int64_t>(x->data)--;
                break;

            case TAG_STRING:
                break;
            }
        }
        break;
    }
    case 0xA8: {
        auto value = progData->bytecode[execData->PC + 1];
        mutateVariable(execData, value, true);
        break;
    }
    case 0xA9: {
        auto value = progData->bytecode[execData->PC + 1];
        mutateVariable(execData, value, false);
        break;
    }
    case 0xB0:
    case 0xB1:
    case 0xB2:
    case 0xB3:
    case 0xB4:
    case 0xB5:
    {
        Variant a, b;
        if(execData->stack.size() < 2) {
            a = {TAG_INT, 0xFEEDFACE};
            b = {TAG_INT, 0xDEADBEEF};
        } else {
            b = execData->stack.back(); execData->stack.pop_back();
            a = execData->stack.back(); execData->stack.pop_back();
        }
        int falseIndex = progData->bytecode[execData->PC + 1];

        bool result = false;

        if (a.type == TAG_STRING && b.type == TAG_STRING) {
            std::string as = std::get<std::string>(a.data);
            std::string bs = std::get<std::string>(b.data);
            switch (opcode) {
            case 0xB0: result = as == bs; break;
            case 0xB1: result = as > bs; break;
            case 0xB2: result = as < bs; break;
            case 0xB3: result = as >= bs; break;
            case 0xB4: result = as <= bs; break;
            case 0xB5: result = as != bs; break;
            }
        } else {
            double av = getNumeric(a);
            double bv = getNumeric(b);
            switch (opcode) {
            case 0xB0: result = av == bv; break;
            case 0xB1: result = av > bv; break;
            case 0xB2: result = av < bv; break;
            case 0xB3: result = av >= bv; break;
            case 0xB4: result = av <= bv; break;
            case 0xB5: result = av != bv; break;
            }
        }

        if (!result) {
            return execData->routineBase + falseIndex;
        }
        break;
    }
    case 0xC0:
    case 0xC1:
    case 0xC2:
    case 0xC3:
    case 0xC4:
    case 0xC5:
    {
        Variant b = execData->stack.back(); execData->stack.pop_back();
        Variant a = execData->stack.back(); execData->stack.pop_back();

        size_t pc = execData->PC + 1;
        uint32_t falseIndex = readU32(progData->bytecode, pc);

        bool result = false;

        if (a.type == TAG_STRING && b.type == TAG_STRING) {
            std::string as = std::get<std::string>(a.data);
            std::string bs = std::get<std::string>(b.data);
            switch (opcode) {
            case 0xC0: result = as == bs; break;
            case 0xC1: result = as > bs; break;
            case 0xC2: result = as < bs; break;
            case 0xC3: result = as >= bs; break;
            case 0xC4: result = as <= bs; break;
            case 0xC5: result = as != bs; break;
            }
        } else {
            double av = getNumeric(a);
            double bv = getNumeric(b);
            switch (opcode) {
            case 0xC0: result = av == bv; break;
            case 0xC1: result = av > bv; break;
            case 0xC2: result = av < bv; break;
            case 0xC3: result = av >= bv; break;
            case 0xC4: result = av <= bv; break;
            case 0xC5: result = av != bv; break;
            }
        }

        if (!result) {
            return execData->routineBase + falseIndex;
        }

        break;
    }
    case 0xAA: {
        int strCount = getInt(execData->stack.back()); execData->stack.pop_back();
        std::string result;
        for (int i = 0; i < strCount; i++) {
            Variant strVar = execData->stack.back(); execData->stack.pop_back();
            std::string str;
            if (strVar.type == TAG_STRING) {
                str = std::get<std::string>(strVar.data);
            } else if (strVar.type == TAG_INT) {
                str = std::to_string(std::get<int64_t>(strVar.data));
            } else if (strVar.type == TAG_FLOAT) {
                str = std::to_string(std::get<double>(strVar.data));
            }
            result = str + result;
        }
        execData->stack.push_back({ TAG_STRING, result });
        break;
    }
    case 0xAB: {
        auto value = progData->bytecode[execData->PC + 1];
        Variant var = execData->stack.back();
        writeVariable(execData, value, var);
        break;
    }
    case 0xDE: {
        Variant ptrVar = execData->stack.back(); execData->stack.pop_back();
        if(ptrVar.type != TAG_INT) {
            std::cout << "Invalid dereference" << std::endl;
            return -1;
        }
        int ptr = getInt(ptrVar);
        const Slot* slot = execData->translator.readSlot(ptr);
        if (!slot) {
            std::cout << "Invalid dereference" << std::endl;
            return -1;
        }
        Variant variable = readVariable(execData, ptr);
        execData->stack.push_back(variable);
        break;
    }
    case 0xFF:
        execData->halt = true;
        break;
    default:
        std::cout << "Invalid opcode: " << (int)opcode << std::endl;
        return -1;
    }

    return execData->PC + offset;
}