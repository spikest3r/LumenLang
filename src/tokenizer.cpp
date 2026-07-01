#include "tokenizer.h"

std::string trim_tabs(const std::string& str) {
    const std::string targets = "\t";
    
    size_t start = str.find_first_not_of(targets);
    if (start == std::string::npos) return "";

    size_t end = str.find_last_not_of(targets);
    
    return str.substr(start, end - start + 1);
}

std::vector<std::string> tokenizeFormula(std::string formula) {
    std::vector<std::string> tokens;
    std::string token = "";
    bool isQuoteOpen = false;
    trim_tabs(formula);
    for (char c : formula) {
        if (c == '(' || c == ')') {
            if(!isQuoteOpen) {
                if (token.length() > 0) tokens.push_back(token);
                tokens.push_back(std::string(1, c));
                token = "";
                continue;
            }
        } else if (c == ',') {
            if(!isQuoteOpen) {
                if (token.length() > 0) tokens.push_back(token);
                tokens.push_back(",");
                token = "";
                continue;
            }
        } else if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%') {
            if(!isQuoteOpen) {
                if (token.length() > 0) tokens.push_back(token);
                tokens.push_back(std::string(1, c));
                token = "";
                continue;
            }
        } else if (c == ' ') {
            if(!isQuoteOpen) {
                if (token.length() > 0) tokens.push_back(token);
                token = "";
                continue;
            }
        } else if(c == '\'') {
            isQuoteOpen = !isQuoteOpen;
            if(!isQuoteOpen) {
                if (token.length() > 0) tokens.push_back(token + '\'');
                token = "";
                continue;
            }
        } else if(c == '#') {
            // comment
            break;
        }
        token += c;
    }
    if (token.length() != 0) {
        tokens.push_back(token);
    }
    return tokens;
}