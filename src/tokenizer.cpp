#include "tokenizer.h"
#include <algorithm>
#include <cctype>

namespace {

std::string trim_tabs(const std::string& str) {
    const std::string targets = "\t";
    size_t start = str.find_first_not_of(targets);
    if (start == std::string::npos) return "";

    size_t end = str.find_last_not_of(targets);
    return str.substr(start, end - start + 1);
}

bool isIdentChar(unsigned char c) {
    return std::isalnum(c) || c == '_';
}

bool isOperandEndToken(const std::string& t) {
    if (t.empty()) return false;
    if (t == "(" || t == "," || t == "+" || t == "-" || t == "*" || t == "/" ||
        t == "%" || t == " .. " || t == "^" || t == "=" || t == "==" || t == "!=" ||
        t == ">" || t == "<" || t == ">=" || t == "<=" || t == "[" || t == "&" ||
        t == "if" || t == "elif" || t == "while" || t == "return" || t == "repeat") {
        return false;
    }
    return true;
}

} // namespace

std::vector<std::string> tokenizeFormula(std::string formula) {
    std::vector<std::string> tokens;
    std::string token = "";
    bool isQuoteOpen = false;

    formula = trim_tabs(formula);

    auto flush = [&]() {
        if (!token.empty()) {
            tokens.push_back(token);
            token.clear();
        }
    };

    const size_t n = formula.size();
    for (size_t i = 0; i < n; i++) {
        char c = formula[i];

        if (isQuoteOpen) {
            if (c == '\'') {
                isQuoteOpen = false;
                tokens.push_back(token + '\'');
                token.clear();
                continue;
            }
            if (c == '\\' && i + 1 < n) {
                char next = formula[i + 1];
                switch (next) {
                    case 'n': token += '\n'; break;
                    case 't': token += '\t'; break;
                    case '\\': token += '\\'; break;
                    case '\'': token += '\''; break;
                    default: token += next; // or error
                }
                i++;
                continue;
            }
            token += c;
            continue;
        }

        if (c == '\'') {
            flush();
            isQuoteOpen = true;
            token += '\'';   // capture opening quote
            continue;
        }

        if (c == '(' || c == ')' || c == ',') {
            flush();
            tokens.push_back(std::string(1, c));
            continue;
        }

        if (c == '#') {
            break;
        }

        if (std::isspace(static_cast<unsigned char>(c))) {
            flush();
            continue;
        }

        if (c == '.') {
            // '.' inside a numeric literal being built (e.g. "3" + "." -> "3.14") stays glued
            bool numericContext = !token.empty() &&
                std::all_of(token.begin(), token.end(),
                            [](unsigned char ch) { return std::isdigit(ch); });
            if (numericContext) {
                token += c;
                continue;
            }
            // otherwise '.' starts/continues a concat operator '..'
            if (i + 1 < n && formula[i + 1] == '.') {
                flush();
                tokens.push_back(" .. ");
                i++; // consume both dots
                continue;
            }
            // lone '.' with no numeric context and no second dot: treat as ordinary char
            flush();
            token += c;
            flush();
            continue;
        }

        if (c == '&' || c == '*') {
            if (!token.empty() && !isIdentChar(static_cast<unsigned char>(token.back()))) {
                flush();
            }
            if (token.empty() && (c == '&' || !isOperandEndToken(tokens.empty() ? "" : tokens.back()))) {
                token += c;
                continue;
            }
        }

        if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%') {
            flush();
            tokens.push_back(std::string(1, c));
            continue;
        }

        bool tokenIsNumericInProgress = !token.empty() && token.back() == '.' &&
            token.size() >= 2 &&
            std::all_of(token.begin(), token.end() - 1,
                        [](unsigned char ch) { return std::isdigit(ch); });

        if (isIdentChar(static_cast<unsigned char>(c))) {
            bool pendingPrefix = token.size() == 1 && (token[0] == '*' || token[0] == '&');
            if (!token.empty() && !isIdentChar(static_cast<unsigned char>(token.back())) &&
                !tokenIsNumericInProgress && !pendingPrefix) {
                flush();
            }
            token += c;
            continue;
        }

        if (!token.empty() && isIdentChar(static_cast<unsigned char>(token.back()))) {
            flush();
        }
        token += c;
    }

    if (isQuoteOpen) {
        flush();
    } else {
        flush();
    }

    return tokens;
}