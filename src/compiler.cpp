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
    {"inputInt", 0x03},
    {"inputStr", 0x04},
    {"str2int", 0x05},
    {"int2str", 0x06}
};

void printError(std::string error, int line) {
    std::cerr << "Error on line " << line << std::endl << "  >>> " << error << std::endl;
}

void pushToStack(std::string token, CompilerData* data) {
    data->bytecode.push_back(0x03); // PUSH opcode

    if (token.starts_with("'")) {
        auto strIndex = resolveString(token, data);
        data->bytecode.push_back(0x01); // operand type: string
        data->bytecode.push_back(static_cast<uint8_t>(strIndex));
    } else if (isPureNumber(token)) {
        int x = std::stoi(token);
        int idx = resolveConst(x, data);
        data->bytecode.push_back(0x02); // operand type: int const
        data->bytecode.push_back(static_cast<uint8_t>(idx)); // <-- use idx, not x
    } else {
        bool ref = token.starts_with("&");
        auto index = resolveVariableIndex(token, data);
        data->bytecode.push_back(ref ? 0x04 : 0x03); // <-- use fresh opcodes, not reused 0x02/0x03
        data->bytecode.push_back(static_cast<uint8_t>(index));
    }
}

int compile(const std::string& script, 
    CompilerData* compilerData,
    bool verbose, bool debugInfo
) {
    std::istringstream stream(script);
    std::string line;

    int stringIndex = 0;
    std::unordered_map<std::string, int> variableMap;
    std::unordered_map<std::string, int> stringPoolMap;

    std::unordered_map<std::string, int> jumpTable;
    std::unordered_map<int, std::vector<uint8_t>> subroutineBytecode;
    std::unordered_map<std::string, int> subroutineIndexMap;
    std::vector<UnresolvedJump> unresolvedRoutineCalls;
    std::vector<UnresolvedJump> unresolvedJumps;
    std::vector<int> condJumpStack;

    int blockDepth = 0;
    std::vector<bool> elseDefined;

    int lineIndex = 1; // for user messages
    bool inRoutine = 0;
    int routineIndex = -1;
    int routineCount = 0;

    while (std::getline(stream, line)) {
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
        std::vector<uint8_t>& bytecode = inRoutine ? subroutineBytecode[routineIndex] : compilerData->bytecode;
        for(const auto& token: tokens) {
            if(token == "=") {
                bool fromStack = false;
                if(tokens.size() > 3) {
                    std::string formula;
                    std::vector<std::string> strs;
                    bool allStr = false;
                    bool mixed = false;
                    for(int i = 2; i < tokens.size(); i++) {
                        auto t = tokens[i];
                        if(t == "..") {
                            if(mixed) {
                                printError("Syntax error", lineIndex);
                                return -1;
                            } else {
                                allStr = true;
                            }
                        } else if(t == "+" || t == "-" || t == "*" || t == "/" || t == "%" || t == "^") {
                            if(allStr) {
                                printError("Syntax error", lineIndex);
                                return -1;
                            } else {
                                mixed = true;
                            }
                        }
                    }
                    mixed = false;
                    for(int i = 2; i < tokens.size(); i++) {
                        bool isStr = false;
                        if(tokens[i].starts_with("'")) {
                            isStr = true;
                            if(!allStr && !mixed) {
                                allStr = true;
                            } else if(!allStr && mixed) {
                                printError("Syntax error", lineIndex);
                                return -1;
                            }
                        } else {
                            bool var = false;
                            if(allStr) {
                                var = isVar(tokens[i]);
                                isStr = var;
                            }
                            if(!var) {
                                if(tokens[i] != "..") {
                                    if(allStr && !mixed) {
                                        mixed = true;
                                    } else if(allStr && mixed) {
                                        printError("Syntax error", lineIndex);
                                        return -1;
                                    }
                                }
                            }
                        }

                        if(isStr) {
                            if(tokens[i] != "..") {
                                strs.push_back(tokens[i]);
                            }
                        } else {
                            formula += tokens[i];
                        }
                    }
                    if(allStr) {
                        for(const auto& str: strs) {
                            bytecode.push_back(0x03); // push to stack
                            if(isVar(str)) {
                                bytecode.push_back(0x03); // variable
                                auto varIndex = resolveVariableIndex(str, compilerData);
                                bytecode.push_back(varIndex); // variable index
                            } else {
                                auto strIndex = resolveString(str, compilerData);
                                bytecode.push_back(0x01); // string
                                bytecode.push_back(strIndex); // string index
                            }
                        }
                        
                        // push str count
                        bytecode.push_back(0x03); // push to stack
                        bytecode.push_back(0x04); // raw uint8_t
                        bytecode.push_back(strs.size()); // value

                        bytecode.push_back(0xAA); // join strings
                        fromStack = true;
                    } else if(mixed) {
                        printError("Syntax error", lineIndex);
                        return -1;
                    } else {
                        compileExpression(
                            formula, compilerData
                        ); // result in stack
                        fromStack = true;
                    }
                }
                if(op != NONE) {
                    printError("Syntax error", lineIndex);
                    return -1;
                }
                if(!fromStack) op = ASSIGN;
                if(fromStack) bytecode.push_back(0x02);
                keyword = tokens[0];
                auto var_index = resolveVariableIndex(keyword, compilerData);
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
            else if(token == "halt") {
                if(op != NONE) {
                    printError("Syntax error", lineIndex);
                    return -1;
                }
                bytecode.push_back(0xFF);
                continue;
            } else if(token == "routine") {
                if(op != NONE) {
                    printError("Syntax error", lineIndex);
                    return -1;
                }
                op = SUBROUTINE;
                if(inRoutine) {
                    printError("Nested routines are not allowed", lineIndex);
                    return -1;
                }
                inRoutine = true;
                continue;
            } else if(token == "endroutine") {
                if(op != NONE) {
                    printError("Syntax error", lineIndex);
                    return -1;
                }
                if(!inRoutine) {
                    printError("Unexpected 'endroutine' (no matching 'routine')", lineIndex);
                    return -1;
                }
                bytecode.push_back(0xFE); // RET
                inRoutine = false;
                continue;
            } else if(token == "call") {
                std::string routineName = tokens[1]; // name is always second token
                bytecode.push_back(0x01);
                bytecode.push_back(0xEE);
                unresolvedRoutineCalls.push_back({routineName, (int)(bytecode.size() - 1), lineIndex});
                break;
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
                case SUBROUTINE:
                    {
                        routineIndex = routineCount++;
                        subroutineIndexMap[token] = routineIndex;
                        subroutineBytecode[routineIndex] = std::vector<uint8_t>();
                    }
                    break;
                case ASSIGN:
                    pushToStack(token, compilerData);
                    bytecode.push_back(0x02); // POP
                    bytecode.push_back(varIndex_assign);
                    break;
                case FUNC_CALL:
                case PUSH_STACK:
                    {
                        pushToStack(token, compilerData);
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
                            pushToStack(token, compilerData);
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

    compilerData->bytecode.push_back(0xFF); // HALT

    std::unordered_map<int, int> routineOffsets;

    for(const auto& it : unresolvedJumps) {
            auto keyword = it.keyword;
            auto location = it.location;
            auto line = it.line;

            auto it2 = jumpTable.find(keyword);
            if(it2 != jumpTable.end()) {
                compilerData->bytecode[location] = it2->second;
            } else {
                printError("Label '" + keyword + "' is not defined", line);
                return -1;
            }
        }

        for(const auto& it : subroutineBytecode) {
        auto idx = it.first;
        auto& routineBc = it.second;
        routineOffsets[idx] = compilerData->bytecode.size(); // offset where this routine starts
        compilerData->bytecode.insert(compilerData->bytecode.end(), routineBc.begin(), routineBc.end());
    }

    for(const auto& it : unresolvedRoutineCalls) {
        auto keyword = it.keyword;
        auto location = it.location;
        auto line = it.line;

        auto it2 = subroutineIndexMap.find(keyword);

        if(it2 != subroutineIndexMap.end()) {
            int rIdx = it2->second;
            compilerData->bytecode[location] = routineOffsets[rIdx]; // patch with actual byte offset
        } else {
            printError("Subroutine '" + keyword + "' is not defined", line);
            return -1;
        }
    }

    // TODO: Debug info

    // if(debugInfo) {
    //     std::ofstream debugFile(fileName + ".bin.dbg");
    //     // write variable names and their indices
    //     debugFile << "variables" << std::endl;
    //     for(const auto& var : variableMap) {
    //         debugFile << var.first << " " << var.second << std::endl;
    //     }
    //     // write subroutine names, their bytecode offsets and bytecode length
    //     debugFile << "routines" << std::endl;
    //     for(const auto& sub : subroutineIndexMap) {
    //         debugFile << sub.first << std::endl;
    //         debugFile << routineOffsets[sub.second] - 1 << std::endl;
    //         debugFile << subroutineBytecode[sub.second].size() << std::endl;
    //     }
    //     // write exec functions
    //     debugFile << "exec" << std::endl;
    //     for(const auto& func: funcList) {
    //         debugFile << func.first << " " << func.second << std::endl;
    //     }
    // }

    return 0;
}