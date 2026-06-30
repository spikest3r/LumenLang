#include "compiler.h"

struct UnresolvedJump {
    std::string keyword;
    int location;
};

int compile(std::string fileName, 
    std::vector<int>& bytecode,
    std::unordered_map<std::string, int>& variableMap,
    std::vector<std::string>& stringPool, std::unordered_map<std::string, int>& stringPoolMap,
    const std::unordered_map<std::string, int>& funcList,
    int& variableIndex, int& stringIndex, bool verbose
) {
    std::ifstream file(fileName);
    std::string line;

    std::unordered_map<std::string, int> jumpTable;
    std::vector<UnresolvedJump> unresolvedJumps;

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
                    std::cerr << "Not NONE" << std::endl;
                    return -1;
                }
                if(!fromStack) op = ASSIGN;
                bytecode.push_back(fromStack ? 0x02 : 0x01); // assign
                auto var_index = resolveVariableIndex(keyword, variableMap, variableIndex);
                bytecode.push_back(var_index);
                if(fromStack) break;
                continue;
            } else if(token == "label") {
                if(op != NONE) {
                    std::cerr << "Not NONE 3" << std::endl;
                    return -1;
                }
                op = LABEL;
                continue;
            } else if(token == "jump") {
                if(op != NONE) {
                    std::cerr << "Not NONE 4" << std::endl;
                    return -1;
                }
                op = JUMP;
                continue;
            }
            else {
                if(op == NONE) {
                    // match function call
                    auto it = funcList.find(token);
                    if (it != funcList.end()) {
                        if(op != NONE) {
                            std::cerr << "Not NONE 2" << std::endl;
                            return -1;
                        }
                        op = FUNC_CALL;
                        funcIndex = it->second;
                    }
                }
            }

            keyword = token;

            switch(op) {
                case ASSIGN:
                    if(token.starts_with("'")) {
                        auto strIndex = resolveString(token, stringPool, stringPoolMap, stringIndex);
                        bytecode.push_back(0x01);
                        bytecode.push_back(strIndex);
                    } else {
                        // assume int for now
                        int x = std::stoi(token);
                        bytecode.push_back(0x02);
                        bytecode.push_back(x);
                    }
                    break;
                case FUNC_CALL:
                case PUSH_STACK:
                    {
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
                            bytecode.push_back(0xFF);
                            unresolvedJumps.push_back({keyword, (int)(bytecode.size() - 1)});
                        }
                        op = NONE;
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
        }
        op = NONE;
    }

    bytecode.push_back(0xFF); // HALT

    for(const auto& it : unresolvedJumps) {
        auto keyword = it.keyword;
        auto location = it.location;

        auto it2 = jumpTable.find(keyword);
        if(it2 != jumpTable.end()) {
            bytecode[location] = it2->second;
        } else {
            std::cerr << "Undefined jump label!" << std::endl;
            return -1;
        }
    }

    file.close();

    return 0;
}