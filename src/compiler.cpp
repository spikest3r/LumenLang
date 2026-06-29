#include "compiler.h"

int compile(std::string fileName, 
    std::vector<int>& bytecode,
    std::unordered_map<std::string, int>& variableMap,
    std::vector<std::string>& stringPool, std::unordered_map<std::string, int>& stringPoolMap,
    const std::unordered_map<std::string, int>& funcList,
    int& variableIndex, int& stringIndex, bool verbose
) {
    std::ifstream file(fileName);
    std::string line;

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
                    std::cout << "Not NONE" << std::endl;
                    return -1;
                }
                if(!fromStack) op = ASSIGN;
                bytecode.push_back(fromStack ? 0x02 : 0x01); // assign
                auto var_index = resolveVariableIndex(keyword, variableMap, variableIndex);
                bytecode.push_back(var_index);
                if(fromStack) break;
                continue;
            } else {
                // match function call
                // TODO: Replace with find
                bool matched = false;
                for(const auto& func: funcList) {
                    if(token == func.first) {
                        if(op != NONE) {
                            std::cout << "Not NONE 2" << std::endl;
                            return -1;
                        }
                        op = FUNC_CALL;
                        funcIndex = func.second;
                        matched = true;
                        break;
                    }
                }
                if(matched) continue;
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
                            auto index = resolveVariableIndex(token, variableMap, variableIndex);
                            bytecode.push_back(0x03); // variable
                            bytecode.push_back(index); // variable index
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
        }
        op = NONE;
    }

    bytecode.push_back(0xFF); // HALT

    file.close();

    return 0;
}