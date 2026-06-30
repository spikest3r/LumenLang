#include "vm.h"

int64_t getInt(const Variant& v) {
    return std::get<int64_t>(v.data);
}

int execute(
    const std::vector<int>& bytecode,
    const std::vector<std::string>& stringPool,
    const int& variableIndex
) {
    std::vector<Variant> variables;
    variables.resize(variableIndex);

    std::vector<Variant> stack;

    int PC = 0;
    bool halt = false;

    while(true) {
        auto opcode = bytecode[PC];
        int offset = getOpCodeOffset(opcode);
        switch(opcode) {
            case 0x02:
                {
                    auto varIndex = bytecode[PC + 1];
                    variables[varIndex].type = static_cast<TypeTag>(TAG_INT);
                    variables[varIndex].data = getInt(stack.back());
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
            case 0x04:
                {
                    // TODO: Offload to lookup map function
                    auto functionIndex = bytecode[PC + 1];
                    switch(functionIndex) {
                        case 1:
                        case 2:
                            {
                                auto arg0 = stack.back();
                                stack.pop_back();
                                std::visit([](const auto& val) {
                                    std::cout << val;
                                }, arg0.data);
                                if(functionIndex == 1) std::cout << std::endl;
                            }
                            break;
                        case 3:
                            {
                                std::string input;
                                auto arg0 = getInt(stack.back()); // holds variable index
                                stack.pop_back();
                                std::cin >> input;
                                int result = 0;
                                try {
                                    result = std::atoi(input.c_str());
                                } catch(...) {
                                    std::cout << "Invalid value!" << std::endl;
                                }
                                variables[arg0].type = TAG_INT;
                                variables[arg0].data = result;
                            }
                            break;
                    }
                }
                break;
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