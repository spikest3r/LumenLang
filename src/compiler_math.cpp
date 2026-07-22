#include "compiler.h"

static bool isNum(const std::string &s) {
    if (s.empty()) return false;
    try {
        size_t pos = 0;
        std::stod(s, &pos);
        return pos == s.size(); // reject partial parses like "1.2.3" or "3abc"
    } catch (...) {
        return false;
    }
}

static bool isOp(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '^' || c == '%';
}

static int getPrec(char c) {
    if (c == '+' || c == '-') return 1;
    if (c == '*' || c == '/' || c == '%') return 2;
    if (c == '^') return 3;
    return 0;
}

static bool isRightAssoc(char c) {
    return c == '^';
}

static std::vector<std::string> tokenize(const std::string &expr) {
    std::vector<std::string> tokens;
    std::string buf;
    for (size_t i = 0; i < expr.size(); i++) {
        char ch = expr[i];

        // unary minus: at start, after another operator, or after '('
        if (ch == '-' && (i == 0 || isOp(expr[i - 1]) || expr[i - 1] == '(')) {
            buf += ch;
            continue;
        }

        if ((ch == '+' || ch == '-') && !buf.empty() &&
            (buf.back() == 'e' || buf.back() == 'E') &&
            isNum(buf + "0")) { // buf+"0" e.g. "3e0" parses as a number => buf is a numeric prefix
            buf += ch;
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(ch)) || ch == '.' ||
            std::isalpha(static_cast<unsigned char>(ch)) || ch == '_') {
            buf += ch;
        } else {
            if (!buf.empty()) {
                tokens.push_back(buf);
                buf.clear();
            }
            if (isOp(ch) || ch == '(' || ch == ')')
                tokens.push_back(std::string(1, ch));
        }
    }
    if (!buf.empty())
        tokens.push_back(buf);
    return tokens;
}

static std::vector<std::string> shuntingYard(const std::vector<std::string> &tokens) {
    std::vector<std::string> out;
    std::vector<char> ops;

    for (const std::string &t : tokens) {
        if (isNum(t) || isVar(t)) {
            out.push_back(t);
        } else if (t == "(") {
            ops.push_back('(');
        } else if (t == ")") {
            while (!ops.empty() && ops.back() != '(') {
                out.push_back(std::string(1, ops.back()));
                ops.pop_back();
            }
            if (!ops.empty()) ops.pop_back(); // discard '('
        } else {
            char op = t[0];
            while (!ops.empty() && ops.back() != '(' &&
                   (getPrec(ops.back()) > getPrec(op) ||
                    (getPrec(ops.back()) == getPrec(op) && !isRightAssoc(op)))) {
                out.push_back(std::string(1, ops.back()));
                ops.pop_back();
            }
            ops.push_back(op);
        }
    }
    while (!ops.empty()) {
        out.push_back(std::string(1, ops.back()));
        ops.pop_back();
    }
    return out;
}

static void evalRPN(const std::vector<std::string> &rpn, CompilerData* data, std::vector<uint8_t>& bytecode) {
    std::vector<std::string> stack;

    for (const std::string &t : rpn) {
        if (isNum(t)) {
            bool isFloat = isFloatLiteral(t);
            TypeTag tag = isFloat ? TAG_FLOAT : TAG_INT;
            uint8_t dataType = isFloat ? 0x05 : 0x02;
            double constValue = std::stod(t);
            int constIndex = resolveConst(constValue, tag, data);

            bytecode.push_back(0x03);
            bytecode.push_back(dataType);
            bytecode.push_back(constIndex);
            stack.push_back(t);
        } else if (isVar(t)) {
            int varIndex = resolveVariableIndex(t, data);
            bytecode.push_back(0x03);
            bytecode.push_back(0x03);
            bytecode.push_back(varIndex);
            stack.push_back(t);
        } else {
            if (stack.size() < 2) { return; }
            std::string b = stack.back(); stack.pop_back();
            std::string a = stack.back(); stack.pop_back();

            switch (t[0]) {
                case '+': bytecode.push_back(0xA0); break; // ADD
                case '-': bytecode.push_back(0xA1); break; // SUB
                case '*': bytecode.push_back(0xA2); break; // MUL
                case '/': bytecode.push_back(0xA3); break; // DIV
                case '^': bytecode.push_back(0xA4); break; // POW
                case '%': bytecode.push_back(0xA5); break; // MOD
            }

            stack.push_back(t);
        }
    }
}

void compileExpression(std::string expr, CompilerData* data, std::vector<uint8_t>& bytecode) {
    auto tokens = tokenize(expr);
    auto rpn    = shuntingYard(tokens);
    return evalRPN(rpn, data, bytecode);
}