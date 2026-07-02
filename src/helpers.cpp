#include "helpers.h"

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

int resolveVariableIndex(std::string keyword, std::unordered_map<std::string, int>& variableMap, int& variableIndex) {
    replaceAll(keyword, "&", "");
    auto it = variableMap.find(keyword);

    if (it != variableMap.end()) {
        return it->second;
    } else {
        variableMap[keyword] = variableIndex;
        return variableIndex++;
    }
}

int resolveString(std::string str, std::vector<std::string>& stringPool, std::unordered_map<std::string, int>& stringPoolMap, int& stringIndex) {
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
        // case 0x01:
        //     return 4;
        case 0x03:
            return 3;
        case 0x04:
        case 0x02:
        case 0xB0: // ==
        case 0xB1: // >
        case 0xB2: // <
        case 0xB3: // >=
        case 0xB4: // <=
        case 0xB5: // != 
        case 0x05:
        case 0x01:
            return 2;
        case 0xFF:
        case 0xA0:
        case 0xA1:
        case 0xA2:
        case 0xA3:
        case 0xA4:
        case 0xA5:
        case 0xAA:
        case 0xFE:
            return 1;
    }
    return 0;
}

bool isVar(const std::string &s) {
    if (s.empty()) return false;
    if (std::isdigit(static_cast<unsigned char>(s[0]))) return false; // can't start with digit
    for (char c : s) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') return false;
    }
    return true;
}