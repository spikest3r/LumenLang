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

            case 0x01: // ASSIGN
                {
                    auto varIndex = bytecode[PC + 1];
                    auto varType = bytecode[PC + 2];
                    auto value = bytecode[PC + 3];
                    variables[varIndex].type = static_cast<TypeTag>(varType);
                    switch(varType) {
                        case TAG_INT:
                            variables[varIndex].data = value;
                            break;
                        case TAG_STRING:
                            variables[varIndex].data = stringPool[value];
                            break;
                    }
                }
                break;
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
                    if(functionIndex == 1) {
                        // print
                        auto arg0 = stack.back();
                        stack.pop_back();
                        std::visit([](const auto& val) {
                            std::cout << val << std::endl;
                        }, arg0.data);
                    }
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