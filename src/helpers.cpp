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

int resolveVariableIndex(std::string keyword, CompilerData* data) {
    replaceAll(keyword, "&", "");
    auto it = data->variableMap.find(keyword);

    if (it != data->variableMap.end()) {
        return it->second;
    } else {
        int idx = (int)data->variableMap.size();
        data->variableMap[keyword] = idx;
        return idx;
    }
}

int resolveString(std::string str, CompilerData* data) {
    replaceAll(str, "'", "");

    auto it = data->stringPoolMap.find(str);

    if (it != data->stringPoolMap.end()) {
        return it->second;
    } else {
        int idx = (int)data->stringPool.size();
        data->stringPoolMap[str] = idx;
        data->stringPool.push_back(str);
        return idx;
    }
}

int resolveConst(int constValue, CompilerData* data) {
    auto it = data->constPoolMap.find(constValue);

    if (it != data->constPoolMap.end()) {
        return it->second;
    } else {
        int idx = (int)data->constPool.size();
        data->constPoolMap[constValue] = idx;
        data->constPool.push_back(constValue);
        return idx;
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

std::string variantToString(const Variant& v) {
    switch (v.type) {
        case TAG_INT:
            return std::to_string(std::get<int64_t>(v.data));
        case TAG_STRING:
            return "'" + std::get<std::string>(v.data) + "'";
        default:
            return "<unknown type>";
    }
}