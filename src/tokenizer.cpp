#include "tokenizer.h"

std::vector<std::string> tokenizeFormula(std::string formula) {
    std::vector<std::string> tokens;
    std::string token = "";
    bool isQuoteOpen = false;
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
        }
        token += c;
    }
    if (token.length() != 0) {
        tokens.push_back(token);
    }
    return tokens;
}