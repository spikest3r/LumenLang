#include "compiler.h"
#include <charconv>

static bool isNum(const std::string &s) {
    if (s.empty()) return false;
    float val;
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
    return ec == std::errc() && ptr == s.data() + s.size();
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

static void evalRPN(const std::vector<std::string> &rpn, CompilerData* data) {
    std::vector<std::string> stack;

    auto& bytecode = data->bytecode;

    for (const std::string &t : rpn) {
        if (isNum(t)) {
            bytecode.push_back(0x03);
            bytecode.push_back(0x02); // type 2 = int
            int constValue = std::stoi(t);
            int constIndex = resolveConst(constValue, data);
            bytecode.push_back(constIndex);
            stack.push_back(t);
        } else if (isVar(t)) {
            int varIndex = resolveVariableIndex(t, data);
            bytecode.push_back(0x03);
            bytecode.push_back(0x03); // type 3 = variable
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

            stack.push_back(t); // result placeholder, consumed by later ops if chained
        }
    }
}

void compileExpression(std::string expr, CompilerData* data) {
    auto tokens = tokenize(expr);
    auto rpn    = shuntingYard(tokens);
    return evalRPN(rpn, data);
}