#include <fstream>
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <iomanip>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <variant>

typedef enum {
    TAG_INT = 2,
    TAG_FLOAT = 3,
    TAG_STRING = 1
} TypeTag;

typedef struct {
    TypeTag type;
    std::variant<int64_t, double, std::string> data;
} Variant;

typedef enum {
    NONE,
    ASSIGN,
    FUNC_CALL
} Operation;

std::vector<Variant> variables;
std::unordered_map<std::string, int> variableMap;
std::vector<std::string> stringPool;
std::unordered_map<std::string, int> stringPoolMap;
std::map<std::string, int> funcList = {
    {"print", 0x01}
};

int stringIndex = 0;
int variableIndex = 0;

void replaceAll(std::string& str, const std::string& from, const std::string& to) {
    if (from.empty()) return;
    size_t pos = 0;
    while ((pos = str.find(from, pos)) != std::string::npos) {
        str.replace(pos, from.length(), to);
        pos += to.length();
    }
}

bool isPureNumber(const std::string& s) {
    if (s.empty()) return false;
    size_t start = (s[0] == '-') ? 1 : 0;
    if (start == s.size()) return false; // just "-" alone
    return std::all_of(s.begin() + start, s.end(), [](unsigned char c) {
        return std::isdigit(c);
    });
}

std::vector<std::string> tokenizeFormula(std::string formula) {
    std::vector<std::string> tokens;
    std::string token = "";
    bool isParOpen = false;
    for (char c : formula) {
        if (c == '(' || c == ')') {
            if (token.length() > 0) tokens.push_back(token);
            tokens.push_back(std::string(1, c));
            token = "";
            continue;
        } else if (c == ',') {
            if (token.length() > 0) tokens.push_back(token);
            tokens.push_back(",");
            token = "";
            continue;
        } else if (c == '+' || c == '-' || c == '*' || c == '/') {
            if (token.length() > 0) tokens.push_back(token);
            tokens.push_back(std::string(1, c));
            token = "";
            continue;
        } else if (c == ' ') {
            if (token.length() > 0) tokens.push_back(token);
            token = "";
            continue;
        }
        token += c;
    }
    if (token.length() != 0) {
        tokens.push_back(token);
    }
    return tokens;
}

int resolveVariableIndex(std::string keyword) {
    auto it = variableMap.find(keyword);

    if (it != variableMap.end()) {
        return it->second;
    } else {
        // TODO: Resize vector
        variableMap[keyword] = variableIndex;
        return variableIndex++;
    }
}

int resolveString(std::string str) {
    replaceAll(str, "'", "");

    auto it = stringPoolMap.find(str);

    if (it != stringPoolMap.end()) {
        return it->second;
    } else {
        stringPoolMap[str] = stringIndex;
        stringPool.push_back(str);
        return stringIndex++;
    }
}

int getOpCodeOffset(int opcode) {
    switch(opcode) {
        case 0x01:
            return 4;
        case 0x03:
            return 3;
        case 0x04:
            return 2;
        case 0xFF:
            return 1;
    }
    return 0;
}

int main() {
    std::ifstream file("main.script");
    std::string line;

    std::vector<uint8_t> bytecode;

    while (std::getline(file, line)) {
        std::cout << line << std::endl;
        auto tokens = tokenizeFormula(line);
        std::string keyword = "";
        Operation op = NONE;
        int funcIndex;
        for(const auto& token: tokens) {
            std::cout << "[" << token << "] " << token.size() << " ";

            if(token == "=") {
                if(tokens.size() != 3) {
                    std::cout << "not 3" << std::endl;
                    return -1;
                }
                if(op != NONE) {
                    std::cout << "Not NONE" << std::endl;
                    return -1;
                }
                op = ASSIGN;
                bytecode.push_back(0x01); // assign
                auto var_index = resolveVariableIndex(keyword);
                bytecode.push_back(var_index);
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
                        auto stringIndex = resolveString(token);
                        bytecode.push_back(0x01);
                        bytecode.push_back(stringIndex);
                    } else {
                        // assume int for now
                        int x = std::stoi(token);
                        bytecode.push_back(0x02);
                        bytecode.push_back(x);
                    }
                    break;
                case FUNC_CALL:
                    {
                        bytecode.push_back(0x03); // push to stack
                        if(token.starts_with("'")) {
                            auto stringIndex = resolveString(token);
                            bytecode.push_back(0x01); // string
                            bytecode.push_back(stringIndex); // string index
                        } else if(isPureNumber(token)) {
                            // integer
                            int x = std::stoi(token);
                            bytecode.push_back(0x02); // int
                            bytecode.push_back(x); // value
                        } else {
                            // assume variable
                            auto index = resolveVariableIndex(token);
                            bytecode.push_back(0x03); // variable
                            bytecode.push_back(index); // variable index
                        }
                    }
                    break;
                default:
                    break;
            }
        }
        std::cout << std::endl;
        switch(op) {
            case FUNC_CALL:
                bytecode.push_back(0x04); // call function
                bytecode.push_back(funcIndex);
                break;
        }
        op = NONE;
    }

    bytecode.push_back(0xFF); // HALT

    for (uint8_t b : bytecode) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(b) << " ";
    }
    std::cout << std::dec << std::endl;

    std::cout << "Executing..." << std::endl;
    
    // resize variable vector
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

    std::cout << "Execution finished" << std::endl;

    file.close();
    return 0;
}