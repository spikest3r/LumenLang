#include "compiler.h"

struct UnresolvedJump {
    std::string keyword;
    int location;
    int line;
};

static const std::unordered_map<std::string, ConditionOp> condOpMap = {
    {"==", EQUALS},
    {">",  GREATER},
    {"<",  LESSER},
    {">=", GREATER_OR_EQ},
    {"<=", LESSER_OR_EQ},
    {"!=", NOT_EQUALS}
};

static const std::unordered_map<ConditionOp, int> condOpcodeMap = {
    {EQUALS, 0xB0},
    {GREATER,  0xB1},
    {LESSER,  0xB2},
    {GREATER_OR_EQ, 0xB3},
    {LESSER_OR_EQ, 0xB4},
    {NOT_EQUALS, 0xB5}
};

std::unordered_map<std::string, int> funcList = {
    {"println", 0x01},
    {"print", 0x02},
    {"inputInt", 0x03}
};

std::map<std::string, int> picoFuncList = {
    {"gpioInit", 0x04},
    {"gpioSetDir", 0x05},
    {"gpioPut", 0x06},
    {"sleepMs", 0x07},
    {"gpioGet", 0x08},
    {"gpioPullUp", 0x09},
    {"gpioPullDown", 0x0A}
};

void printError(std::string error, int line) {
    std::cerr << "Error on line " << line << std::endl << "  >>> " << error << std::endl;
}

void pushToStack(std::string token, std::vector<int>& bytecode, 
    std::unordered_map<std::string, int>& variableMap,
    std::vector<std::string>& stringPool, std::unordered_map<std::string, int>& stringPoolMap,
    int& variableIndex, int& stringIndex
) {
    bytecode.push_back(0x03); // push to stack
    if(token.starts_with("'")) {
        auto strIndex = resolveString(token, stringPool, stringPoolMap, stringIndex);
        bytecode.push_back(0x01); // string
        bytecode.push_back(strIndex); // string index
    } else if(isPureNumber(token)) {
        // integer
        int x = std::stoi(token);
        bytecode.push_back(0x02); // int
        bytecode.push_back(x); // value
    } else {
        // assume variable
        bool ref = token.starts_with("&");
        auto index = resolveVariableIndex(token, variableMap, variableIndex);
        bytecode.push_back(ref ? 0x02 : 0x03); // variable
        bytecode.push_back(index); // variable index
    }
}

int compile(std::string fileName, 
    std::vector<int>& bytecode,
    std::unordered_map<std::string, int>& variableMap,
    std::vector<std::string>& stringPool, std::unordered_map<std::string, int>& stringPoolMap,
    int& variableIndex, int& stringIndex, bool verbose, bool picoMode
) {
    if(picoMode) {
        // Add pico-vm specific functions to the function list
        for(const auto& it : picoFuncList) {
            funcList[it.first] = it.second;
        }
    }

    std::ifstream file(fileName);
    std::string line;

    std::unordered_map<std::string, int> jumpTable;
    std::vector<UnresolvedJump> unresolvedJumps;
    std::vector<int> condJumpStack;

    int blockDepth = 0;
    std::vector<bool> elseDefined;

    int lineIndex = 1; // for user messages

    while (std::getline(file, line)) {
        if(verbose) std::cout << line << std::endl;
        auto tokens = tokenizeFormula(line);
        if(verbose) {
            for(const auto& token: tokens) {
                std::cout << "[" << token << "] " << token.size() << " ";
            }
            std::cout << std::endl;
        }
        std::string keyword = "";
        Operation op = NONE;
        int funcIndex;
        int conditionArgs = 0;
        int varIndex_assign = 0;
        ConditionOp condOp = COP_NONE;
        for(const auto& token: tokens) {
            if(token == "=") {
                bool fromStack = false;
                if(tokens.size() > 3) {
                    std::string formula;
                    for(int i = 2; i < tokens.size(); i++) {
                        formula += tokens[i];
                    }
                    compileExpression(
                        formula, bytecode,
                        variableMap, variableIndex
                    ); // result in stack
                    fromStack = true;
                }
                if(op != NONE) {
                    printError("Syntax error", lineIndex);
                    return -1;
                }
                if(!fromStack) op = ASSIGN;
                if(fromStack) bytecode.push_back(0x02);
                keyword = tokens[0];
                auto var_index = resolveVariableIndex(keyword, variableMap, variableIndex);
                varIndex_assign = var_index;
                if(fromStack) bytecode.push_back(var_index);
                if(fromStack) break;
                continue;
            } else if(token == "label") {
                if(op != NONE) {
                    printError("Syntax error", lineIndex);
                    return -1;
                }
                op = LABEL;
                continue;
            } else if(token == "jump") {
                if(op != NONE) {
                    printError("Syntax error", lineIndex);
                    return -1;
                }
                op = JUMP;
                continue;
            } else if(token == "if") {
                if(op != NONE) {
                    printError("Syntax error", lineIndex);
                    return -1;
                }
                op = IF;
                blockDepth++;
                elseDefined.push_back(false);
                continue;
            } else if(token == "endif") {
                if(blockDepth == 0) {
                    printError("Unexpected 'endif' (no matching 'if')", lineIndex);
                    return -1;
                }
                int loc = condJumpStack.back(); condJumpStack.pop_back();
                bytecode[loc] = bytecode.size(); 
                blockDepth--;
                elseDefined.pop_back();
                continue;
            } else if(token == "else") {
                if(blockDepth == 0) {
                    printError("Unexpected 'else' (no matching 'if')", lineIndex);
                    return -1;
                }
                elseDefined[blockDepth - 1] = true;
                int loc = condJumpStack.back();
                bytecode[loc] = bytecode.size() + 2; // patch false jump to point past the upcoming skip-jump
                bytecode.push_back(0x05);
                bytecode.push_back(0xDE);
                condJumpStack.pop_back();
                condJumpStack.push_back(bytecode.size() - 1); // now track the else's skip-jump
                continue;
            }
            else {
                if(op == NONE) {
                    // match function call
                    auto it = funcList.find(token);
                    if (it != funcList.end()) {
                        if(op != NONE) {
                            printError("Syntax error", lineIndex);
                            return -1;
                        }
                        op = FUNC_CALL;
                        funcIndex = it->second;
                    }
                    continue;
                }
            }

            keyword = token;

            switch(op) {
                case ASSIGN:
                    bytecode.push_back(0x03); // PUSH
                    if(token.starts_with("'")) { // string
                        auto strIndex = resolveString(token, stringPool, stringPoolMap, stringIndex);
                        bytecode.push_back(0x01);
                        bytecode.push_back(strIndex);
                    } else if(isVar(token)) {
                        auto varIndex = resolveVariableIndex(token, variableMap, variableIndex);
                        bytecode.push_back(0x03);
                        bytecode.push_back(varIndex);
                    } 
                    else {
                        // assume int for now
                        int x = std::stoi(token);
                        bytecode.push_back(0x02);
                        bytecode.push_back(x);
                    }
                    bytecode.push_back(0x02); // POP
                    bytecode.push_back(varIndex_assign);
                    break;
                case FUNC_CALL:
                case PUSH_STACK:
                    {
                        pushToStack(token, bytecode, variableMap, stringPool, stringPoolMap, variableIndex, stringIndex);
                    }
                    break;
                case LABEL:
                    {
                        jumpTable[keyword] = bytecode.size(); // point to next instruction
                        op = NONE;
                    }
                    break;
                case JUMP:
                    {
                        auto it = jumpTable.find(keyword);
                        if(it != jumpTable.end()) {
                            bytecode.push_back(0x05);
                            bytecode.push_back(it->second);
                        } else {
                            bytecode.push_back(0x05);
                            bytecode.push_back(0xBE);
                            unresolvedJumps.push_back({keyword, (int)(bytecode.size() - 1), lineIndex});
                        }
                        op = NONE;
                    }
                    break;
                case IF:
                    {
                        auto it = condOpMap.find(keyword);
                        if (it != condOpMap.end()) {
                            if (conditionArgs != 1) {
                                printError("Syntax error", lineIndex);
                                return -1;
                            }
                            condOp = it->second;
                        } else {
                            if(conditionArgs > 1 && condOp != COP_NONE) {
                                printError("Syntax error", lineIndex);
                                return -1;
                            }
                            pushToStack(token, bytecode, variableMap, stringPool, stringPoolMap, variableIndex, stringIndex);
                            conditionArgs++;
                        }
                    }
                    break;
                default:
                    break;
            }
        }
        switch(op) {
            case FUNC_CALL:
                bytecode.push_back(0x04); // call function
                bytecode.push_back(funcIndex);
                break;
            case IF:
                {
                    auto it = condOpcodeMap.find(condOp);
                    if (it != condOpcodeMap.end()) {
                        if (conditionArgs != 2) {
                            printError("Syntax error", lineIndex);
                            return -1;
                        }
                        bytecode.push_back(it->second);
                        bytecode.push_back(0xBF);
                        condJumpStack.push_back(bytecode.size() - 1);
                    } else {
                        printError("Syntax error", lineIndex);
                        return -1;
                    }
                }
                break;
        }
        op = NONE;

        lineIndex++;
    }

    bytecode.push_back(0xFF); // HALT

    for(const auto& it : unresolvedJumps) {
        auto keyword = it.keyword;
        auto location = it.location;
        auto line = it.line;

        auto it2 = jumpTable.find(keyword);
        if(it2 != jumpTable.end()) {
            bytecode[location] = it2->second;
        } else {
            printError("Label '" + keyword + "' is not defined", line);
            return -1;
        }
    }

    file.close();

    return 0;
}