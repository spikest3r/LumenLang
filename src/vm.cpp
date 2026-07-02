#include "vm.h"

int64_t getInt(const Variant& v) {
    return std::get<int64_t>(v.data);
}

using NativeFn = std::function<void(std::vector<Variant>&, std::vector<Variant>&)>;

std::unordered_map<int, NativeFn> funcMap = {
    {0x01, [](std::vector<Variant>& stack, std::vector<Variant>& variables) {
        auto arg0 = stack.back(); stack.pop_back();
        std::visit([](const auto& val) { std::cout << val; }, arg0.data);
        std::cout << std::endl;
    }},
    {0x02, [](std::vector<Variant>& stack, std::vector<Variant>& variables) {
        auto arg0 = stack.back(); stack.pop_back();
        std::visit([](const auto& val) { std::cout << val; }, arg0.data);
    }},
    {0x03, [](std::vector<Variant>& stack, std::vector<Variant>& variables) {
        auto varIndex = getInt(stack.back()); stack.pop_back();
        std::string input;
        std::cin >> input;
        int64_t result = 0;
        try {
            result = std::stoll(input);
        } catch(...) {
            std::cout << "Invalid value!" << std::endl;
        }
        variables[varIndex].type = TAG_INT;
        variables[varIndex].data = result;
    }}
};

int execute(
    const std::vector<int>& bytecode,
    const std::vector<std::string>& stringPool,
    const int& variableIndex
) {
    std::vector<Variant> variables;
    variables.resize(variableIndex);

    std::vector<Variant> stack;
    std::vector<int> pcStack;

    int PC = 0;
    bool halt = false;

    while(true) {
        auto opcode = bytecode[PC];
        int offset = getOpCodeOffset(opcode);
        switch(opcode) {
            case 0x01:
                // run subroutine
                {
                    auto addr = bytecode[PC + 1];
                    pcStack.push_back(PC + offset);
                    PC = addr;
                    continue;
                }
                break;
            case 0xFE:
                // return from subroutine
                {
                    if(pcStack.empty()) {
                        std::cerr << "Return stack underflow!" << std::endl;
                        return -1;
                    }
                    PC = pcStack.back();
                    pcStack.pop_back();
                    continue;
                }
                break;
            case 0x02:
                {
                    auto varIndex = bytecode[PC + 1];
                    auto var = stack.back();
                    variables[varIndex].type = var.type;
                    switch(var.type) {
                        case TAG_INT:
                            variables[varIndex].data = std::get<int64_t>(var.data);
                            break;
                        case TAG_STRING:
                            variables[varIndex].data = std::get<std::string>(var.data);
                            break;
                    }
                    stack.pop_back();
                }
                break;
            case 0x03:
                {
                    auto dataType = bytecode[PC + 1];
                    auto value = bytecode[PC + 2];
                    switch(dataType) {
                        case 0x01:
                            stack.push_back({TAG_STRING, stringPool[value]});
                            break;
                        case 0x02:
                            stack.push_back({TAG_INT, value});
                            break;
                        case 0x03:
                            {
                                auto variable = variables[value];
                                stack.push_back({variable.type, variable.data});
                            }
                            break;
                    }
                }
                break;
            case 0x04: {
                auto functionIndex = bytecode[PC + 1];
                auto it = funcMap.find(functionIndex);
                if (it != funcMap.end()) {
                    it->second(stack, variables);
                } else {
                    if(functionIndex > 0x03 && functionIndex < 0x0B) { // 0x04 to 0x0A are pico-vm specific functions
                        std::cerr << "Pico VM: " << functionIndex << std::endl;
                    } else {
                        std::cerr << "Unknown function index: " << functionIndex << std::endl;
                        return -1;
                    }
                }
                break;
            }
            case 0x05:
                {
                    auto newPC = bytecode[PC + 1];
                    PC = newPC;
                    continue;
                }
                break;
            case 0xA0: { // ADD
                Variant b = stack.back(); stack.pop_back();
                Variant a = stack.back(); stack.pop_back();
                Variant result;
                result.type = TAG_INT;
                result.data = getInt(a) + getInt(b);
                stack.push_back(result);
                break;
            }
            case 0xA1: { // SUB
                Variant b = stack.back(); stack.pop_back();
                Variant a = stack.back(); stack.pop_back();
                Variant result;
                result.type = TAG_INT;
                result.data = getInt(a) - getInt(b);
                stack.push_back(result);
                break;
            }
            case 0xA2: { // MUL
                Variant b = stack.back(); stack.pop_back();
                Variant a = stack.back(); stack.pop_back();
                Variant result;
                result.type = TAG_INT;
                result.data = getInt(a) * getInt(b);
                stack.push_back(result);
                break;
            }
            case 0xA3: { // DIV
                Variant b = stack.back(); stack.pop_back();
                Variant a = stack.back(); stack.pop_back();
                Variant result;
                result.type = TAG_INT;
                result.data = getInt(a) / getInt(b);
                stack.push_back(result);
                break;
            }
            case 0xA4: { // POW
                Variant b = stack.back(); stack.pop_back();
                Variant a = stack.back(); stack.pop_back();
                Variant result;
                result.type = TAG_INT;
                result.data = static_cast<int64_t>(std::pow(getInt(a), getInt(b)));
                stack.push_back(result);
                break;
            }
            case 0xA5: { // MOD
                Variant b = stack.back(); stack.pop_back();
                Variant a = stack.back(); stack.pop_back();
                Variant result;
                result.type = TAG_INT;
                result.data = getInt(a) % getInt(b);
                stack.push_back(result);
                break;
            }
            case 0xB0: // ==
            case 0xB1: // >
            case 0xB2: // <
            case 0xB3: // >=
            case 0xB4: // <=
            case 0xB5: // != 
            {
                Variant b = stack.back(); stack.pop_back();
                Variant a = stack.back(); stack.pop_back();
                int falseIndex = bytecode[PC + 1];
                
                bool result = false;
                int64_t av = getInt(a);
                int64_t bv = getInt(b);
                
                switch (opcode) {
                    case 0xB0: result = av == bv; break;
                    case 0xB1: result = av >  bv; break;
                    case 0xB2: result = av <  bv; break;
                    case 0xB3: result = av >= bv; break;
                    case 0xB4: result = av <= bv; break;
                    case 0xB5: result = av != bv; break;
                }
                
                if (!result) {
                    PC = falseIndex;
                    continue;
                }
                break;
            }
            case 0xAA: { // join strings
                int strCount = getInt(stack.back()); stack.pop_back();
                std::string result;
                for(int i = 0; i < strCount; i++) {
                    Variant strVar = stack.back(); stack.pop_back();
                    std::string str = std::get<std::string>(strVar.data);
                    result = str + result; // prepend to maintain order
                }
                stack.push_back({TAG_STRING, result});
                break;
            }
            case 0xFF:
                halt = true;
                break;
            default:
                std::cout << "Invalid opcode" << std::endl;
                return -1; // error
        }
        if(halt) break;
        PC += offset;
    }
    return 0;
}