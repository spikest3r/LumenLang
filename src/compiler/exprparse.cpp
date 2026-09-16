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

static bool isOp(const std::string &s) {
    if (s == "+" || s == "-" || s == "*" || s == "/" || s == "^" || s == "%" || s == "..")
        return true;
    return false;
}

static bool isOp(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '^' || c == '%';
    // '..' can't be represented as a single char, so its excluded here
}

static int getPrec(const std::string &op) {
    if (op == "..") return 1;
    if (op == "+" || op == "-") return 2;
    if (op == "*" || op == "/" || op == "%") return 3;
    if (op == "^") return 4;
    return 0;
}

static bool isRightAssoc(const std::string &op) {
    return op == "^";
}

static std::vector<std::string> tokenize(const std::string &expr) {
    std::vector<std::string> tokens;
    std::string buf;
    for (size_t i = 0; i < expr.size(); i++) {
        char ch = expr[i];

        // string literal: consume until matching closing quote
        if (ch == '\'' || ch == '"') {
            if (!buf.empty()) {
                tokens.push_back(buf);
                buf.clear();
            }
            char quote = ch;
            std::string literal;
            literal += quote; // keep opening quote
            size_t j = i + 1;
            for (; j < expr.size() && expr[j] != quote; j++) {
                literal += expr[j];
            }
            if (j < expr.size()) literal += expr[j]; // keep closing quote
            tokens.push_back(literal);
            i = j;
            continue;
        }

        // unary minus: at start, after another operator, or after '('
        if (ch == '-' && (i == 0 || isOp(expr[i - 1]) || expr[i - 1] == '(')) {
            buf += ch;
            continue;
        }

        // dereference 
        if (ch == '*' && buf.empty() &&
            i + 1 < expr.size() &&
            (std::isalpha(static_cast<unsigned char>(expr[i + 1])) ||
            expr[i + 1] == '_' || expr[i + 1] == '&')) {
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
            std::isalpha(static_cast<unsigned char>(ch)) || ch == '_' || ch == '&') {
            buf += ch;
        } else {
            if (!buf.empty()) {
                tokens.push_back(buf);
                buf.clear();
            }
            if (isOp(ch) || ch == '(' || ch == ')' || ch == ',')
                tokens.push_back(std::string(1, ch));
        }
    }
    if (!buf.empty())
        tokens.push_back(buf);
    return tokens;
}

static bool isStringLit(const std::string &t) {
    return t.size() >= 2 && (t.front() == '\'' || t.front() == '"') &&
           t.back() == t.front();
}

extern std::unordered_map<std::string, Function> funcList;

static std::string makeCallToken(const std::string &name, int argCount) {
    return "@call:" + name + ":" + std::to_string(argCount);
}

static bool isCallToken(const std::string &t) {
    return t.size() > 6 && t.compare(0, 6, "@call:") == 0;
}

// Splits "@call:name:argCount" back into its parts. Assumes isCallToken(t).
static void parseCallToken(const std::string &t, std::string &name, int &argCount) {
    size_t nameStart = 6; // strlen("@call:")
    size_t lastColon = t.rfind(':');
    name = t.substr(nameStart, lastColon - nameStart);
    argCount = std::stoi(t.substr(lastColon + 1));
}

// Tracks state for one open call's argument list so nested calls
// (foo(bar(1,2), 3)) resolve independently on their own ')'.
struct CallArgFrame {
    std::string name;
    int argCount; // commas seen so far at this call's nesting level
};

// True if `name` resolves to either a native function or a declared routine.
static bool isCallable(const std::string &name, CompilerData* data) {
    return funcList.find(name) != funcList.end() ||
           data->routineList.find(name) != data->routineList.end();
}

static int requiredArgCount(const std::string &name, CompilerData* data) {
    auto fIt = funcList.find(name);
    if (fIt != funcList.end()) return fIt->second.argCount;
    auto rIt = data->routineList.find(name);
    if (rIt != data->routineList.end()) return rIt->second.argCount;
    throw std::runtime_error("unknown function or routine: " + name);
}

static std::vector<std::string> shuntingYard(const std::vector<std::string> &tokens, CompilerData* data) {
    std::vector<std::string> out;
    std::vector<std::string> ops;      // operators AND "(" markers
    std::vector<CallArgFrame> callStack;  // parallel to any "(" that opens a call
    std::vector<bool> parenIsCall;     // parallel to ops: was this "(" a call-open?

    for (size_t i = 0; i < tokens.size(); i++) {
        const std::string &t = tokens[i];

        if (isNum(t) || isVar(t) || isStringLit(t)) {
            out.push_back(t);
        } else if (t == "(") {
            // A "(" immediately after an identifier that isn't itself a
            // variable/number/etc is a call open, e.g. "foo" then "(".
            bool isCallOpen = false;
            if (i > 0) {
                const std::string &prev = tokens[i - 1];
                if (!isNum(prev) && !isStringLit(prev) && !isOp(prev) &&
                    prev != "(" && prev != ")" && prev != "," &&
                    !prev.empty() &&
                    (std::isalpha(static_cast<unsigned char>(prev[0])) || prev[0] == '_')) {
                    isCallOpen = true;
                }
            }

            if (isCallOpen) {
                const std::string &name = tokens[i - 1];
                if (!isCallable(name, data)) {
                    throw std::runtime_error("unknown function or routine: " + name);
                }
                if (!out.empty() && out.back() == name) out.pop_back();

                callStack.push_back(CallArgFrame{name, 0});
                parenIsCall.push_back(true);
            } else {
                parenIsCall.push_back(false);
            }
            ops.push_back(t);
        } else if (t == ",") {
            if (callStack.empty()) {
                throw std::runtime_error("',' outside of a function call");
            }
            while (!ops.empty() && ops.back() != "(") {
                out.push_back(ops.back());
                ops.pop_back();
            }
            if (ops.empty()) {
                throw std::runtime_error("mismatched ',' in expression");
            }
            callStack.back().argCount++;
        } else if (t == ")") {
            while (!ops.empty() && ops.back() != "(") {
                out.push_back(ops.back());
                ops.pop_back();
            }
            if (ops.empty()) {
                throw std::runtime_error("mismatched ')' in expression");
            }
            ops.pop_back(); // discard '('
            bool wasCall = !parenIsCall.empty() && parenIsCall.back();
            if (!parenIsCall.empty()) parenIsCall.pop_back();

            if (wasCall) {
                CallArgFrame frame = callStack.back();
                callStack.pop_back();

                bool emptyArgs = (i > 0 && tokens[i - 1] == "(");
                int finalArgCount = emptyArgs ? 0 : frame.argCount + 1;

                const std::string &name = frame.name;
                int expectedArgCount = requiredArgCount(name, data);
                if (expectedArgCount != finalArgCount) {
                    throw std::runtime_error(
                        "argument count mismatch for '" + name + "': expected " +
                        std::to_string(expectedArgCount) + ", got " +
                        std::to_string(finalArgCount));
                }

                out.push_back(makeCallToken(name, finalArgCount));
            }
        } else {
            const std::string &op = t;

            // type check
            if ((op == "+" || op == "-" || op == "*" || op == "/") &&
                !out.empty() && isStringLit(out.back())) {
                throw std::runtime_error("type error: arithmetic operator '" + op +
                                          "' cannot be applied to string literal " + out.back());
            }
            
            // type check
            if (op == ".." && !out.empty() && isNum(out.back())) {
                throw std::runtime_error("type error: '..' cannot be applied to numeric literal " + out.back());
            }
            
            while (!ops.empty() && ops.back() != "(" &&
                (getPrec(ops.back()) > getPrec(op) ||
                    (getPrec(ops.back()) == getPrec(op) && !isRightAssoc(op)))) {
                out.push_back(ops.back());
                ops.pop_back();
            }

            ops.push_back(op);
        }
    }
    while (!ops.empty()) {
        if (ops.back() == "(") {
            throw std::runtime_error("mismatched '(' in expression");
        }
        out.push_back(ops.back());
        ops.pop_back();
    }
    return out;
}

static void evalRPN(const std::vector<std::string> &rpn, CompilerData* data, std::vector<uint8_t>& bytecode) {
    std::vector<std::string> stack;

    for (const std::string &t : rpn) {
        if (isCallToken(t)) {
            std::string name;
            int argCount;
            parseCallToken(t, name, argCount);

            if (static_cast<int>(stack.size()) < argCount) {
                throw std::runtime_error(
                    "internal error: not enough operands on stack for call to '" +
                    name + "'");
            }
            for (int a = 0; a < argCount; a++) stack.pop_back();

            auto fIt = funcList.find(name);
            if (fIt != funcList.end()) {
                if(!fIt->second.returnable) {
                    throw std::runtime_error(std::format(
                        "error: '{}' does not return a value", name
                    ));
                }
                bytecode.push_back(0x04); // call native function
                bytecode.push_back(static_cast<uint8_t>(fIt->second.opcode));
            } else {
                auto rIt = data->routineList.find(name);
                if (rIt == data->routineList.end()) {
                    throw std::runtime_error("unknown function or routine: " + name);
                }
                if(!rIt->second.returnable) {
                    throw std::runtime_error(std::format(
                        "error: '{}' does not return a value", name
                    ));
                }

                bytecode.push_back(0x07); // CALL32
                int loc = static_cast<int>(bytecode.size());
                bytecode.push_back(0x00);
                bytecode.push_back(0x00);
                bytecode.push_back(0x00);
                bytecode.push_back(0x00);

                data->pendingRoutineCalls.push_back(
                    PendingRoutineCall{name, loc, data->currentRoutineIndex, argCount});
            }

            stack.push_back(t); // placeholder for the call's return value
        } else if (isStringLit(t)) {
            std::string value = t.substr(1, t.size() - 2); // strip quotes
            int constIndex = resolveString(value, data);

            bytecode.push_back(0x03);
            bytecode.push_back(0x01); // TAG_STRING
            bytecode.push_back(constIndex);
            stack.push_back(t); // keep quotes so isStringLit still matches later
        } else if (isNum(t)) {
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
            bool deref = t.starts_with("*");
            std::string rest = deref ? t.substr(1) : t;
            bool ref = rest.starts_with("&"); // resolveVariableIndex strips '&' itself

            int varIndex = resolveVariableIndex(rest, data);
            bytecode.push_back(0x03);
            bytecode.push_back(ref ? 0x04 : 0x03);
            bytecode.push_back(static_cast<uint8_t>(varIndex));

            if (deref) {
                bytecode.push_back(0xDE); // dereference: pointer on stack -> value on stack
            }

            stack.push_back(t);
        } else {
            if (stack.size() < 2) { return; }
            std::string b = stack.back(); stack.pop_back();
            std::string a = stack.back(); stack.pop_back();

            bool aIsStr = isStringLit(a);
            bool bIsStr = isStringLit(b);

            if (t == "..") {
                // concat: literal operands must not be numeric
                if ((isNum(a) && !aIsStr) || (isNum(b) && !bIsStr)) {
                    throw std::runtime_error("type error: '..' cannot be applied to a numeric literal");
                }

                // push count
                bytecode.push_back(0x03);
                bytecode.push_back(0x04);
                bytecode.push_back(2); // string count for JOIN

                bytecode.push_back(0xAA); // JOIN
            } else {
                // arithmetic: literal operands must not be strings
                if (aIsStr || bIsStr) {
                    throw std::runtime_error("type error: arithmetic operator '" + t +
                                              "' cannot be applied to a string literal");
                }
                switch (t[0]) {
                    case '+': bytecode.push_back(0xA0); break; // ADD
                    case '-': bytecode.push_back(0xA1); break; // SUB
                    case '*': bytecode.push_back(0xA2); break; // MUL
                    case '/': bytecode.push_back(0xA3); break; // DIV
                    case '^': bytecode.push_back(0xA4); break; // POW
                    case '%': bytecode.push_back(0xA5); break; // MOD
                }
            }

            stack.push_back(t);
        }
    }
}

void compileExpression(std::string expr, CompilerData* data, std::vector<uint8_t>& bytecode) {
    auto tokens = tokenize(expr);
    auto rpn    = shuntingYard(tokens, data);
    return evalRPN(rpn, data, bytecode);
}