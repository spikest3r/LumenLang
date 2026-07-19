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

    bool seenDigit = false;
    bool seenDot = false;
    bool seenExp = false;
    for (size_t i = start; i < s.size(); i++) {
        unsigned char c = s[i];
        if (std::isdigit(c)) {
            seenDigit = true;
        } else if (c == '.' && !seenDot && !seenExp) {
            seenDot = true;
        } else if ((c == 'e' || c == 'E') && !seenExp && seenDigit) {
            seenExp = true;
            // optional sign right after the exponent marker
            if (i + 1 < s.size() && (s[i + 1] == '+' || s[i + 1] == '-')) {
                i++;
            }
        } else {
            return false;
        }
    }
    return seenDigit;
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

int resolveConst(double constValue, TypeTag type, CompilerData* data) {
    std::pair<int, double> key = {static_cast<int>(type), constValue};

    auto it = data->constPoolMap.find(key);
    if (it != data->constPoolMap.end()) {
        return it->second;
    }

    int index = static_cast<int>(data->constPool.size());
    data->constPool.push_back(constValue);
    data->constPoolMap[key] = index;
    return index;
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
        case TAG_FLOAT:
            return std::to_string(std::get<double>(v.data));
        case TAG_STRING:
            return "'" + std::get<std::string>(v.data) + "'";
        default:
            return "<unknown type>";
    }
}

// A literal is a float if it contains a decimal point or an exponent.
// "3" -> int, "3.0" -> float, "3e2" -> float, "-3.5" -> float.
bool isFloatLiteral(const std::string &s) {
    return s.find('.') != std::string::npos ||
           s.find('e') != std::string::npos ||
           s.find('E') != std::string::npos;
}

std::map<int, std::string> disassemblyMap = {
    {0x01, "CALL"},
    {0x02, "POP"},
    {0x03, "PUSH"},
    {0x04, "EXEC"},
    {0x05, "JUMP"},
    
    {0xA0, "ADD"},
    {0xA1, "SUB"},
    {0xA2, "MUL"},
    {0xA3, "DIV"},
    {0xA4, "POW"},
    {0xA5, "MOD"},
    
    {0xB0, "JEQ"},
    {0xB1, "JGR"},
    {0xB2, "JLS"},
    {0xB3, "JGE"},
    {0xB4, "JLE"},
    {0xB5, "JNE"},

    {0xAA, "JOIN"},

    {0xFE, "RET"},
    {0xFF, "HLT"}
};