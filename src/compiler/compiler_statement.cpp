#include "compiler_internal.h"
#include <cctype>

int compileLine(const std::string& currentLine, CompileState& state, CompilerData* compilerData,
    bool verbose, bool debugInfo
) {
    std::string line = currentLine;
    
    // Pre-process strings to protect them from destructive tokenization
    std::vector<std::string> stringLiterals;
    std::string modifiedLine;
    bool inStr = false;
    char quoteChar = 0;
    std::string currentStr;
    
    for (size_t i = 0; i < line.size(); i++) {
        char c = line[i];
        if (!inStr) {
            if (c == '"' || c == '\'') {
                inStr = true;
                quoteChar = c;
                currentStr = c;
            } else {
                modifiedLine += c;
            }
        } else {
            currentStr += c;
            if (c == quoteChar) {
                int backslashes = 0;
                for (int j = static_cast<int>(i) - 1; j >= 0 && line[j] == '\\'; j--) {
                    backslashes++;
                }
                if (backslashes % 2 == 0) { 
                    inStr = false;
                    std::string placeholder = "STRLIT" + std::to_string(stringLiterals.size());
                    stringLiterals.push_back(currentStr);
                    modifiedLine += placeholder;
                }
            }
        }
    }
    if (inStr) {
        std::string placeholder = "STRLIT" + std::to_string(stringLiterals.size());
        stringLiterals.push_back(currentStr);
        modifiedLine += placeholder;
    }

    if (verbose) std::cout << "Prescanned: " << modifiedLine << std::endl;
    auto rawTokens = tokenizeFormula(modifiedLine);
    
    // Swap placeholders back for exact intact string literals
    std::vector<std::string> tokens;
    for (const auto& t : rawTokens) {
        if (t.starts_with("STRLIT") && t.size() > 6) {
            bool allDigits = true;
            for(size_t k = 6; k < t.size(); k++) {
                if(!std::isdigit(static_cast<unsigned char>(t[k]))) {
                    allDigits = false;
                    break;
                }
            }
            if(allDigits) {
                int idx = std::stoi(t.substr(6));
                if (idx >= 0 && idx < stringLiterals.size()) {
                    tokens.push_back(stringLiterals[idx]);
                    continue;
                }
            }
        }
        tokens.push_back(t);
    }

    if (verbose) {
        for (const auto& token : tokens) {
            std::cout << "[" << token << "] " << token.size() << " ";
        }
        std::cout << std::endl;
    }
    std::string keyword = "";
    Operation op = NONE;
    int funcIndex = 0;
    int conditionArgs = 0;
    int varIndex_assign = 0;
    ConditionOp condOp = COP_NONE;
    bool isMainBody = !(state.inRoutine || state.inFunction);
    std::vector<uint8_t>& bytecode = isMainBody ? compilerData->bytecode : state.subroutineBytecode[state.routineIndex];
    compilerData->currentRoutineIndex = isMainBody ? -1 : state.routineIndex;
    std::vector<std::string> tokenStack; // for temporary holds, cleared on every new line
    bool argsOpen = false;
    int callParenDepth = 0;
    std::string routineToCall = "";

    for (const auto& token : tokens) {
        if (token == "=") {
            std::string formula;
            std::vector<std::string> strs;
            
            for (size_t i = 2; i < tokens.size(); i++) {
                formula += tokens[i] + " ";
            }

            try {
                compileExpression(
                    formula, compilerData, bytecode
                ); // result in stack
            } catch (const std::exception& e) {
                printError(e.what(), state.lineIndex, state.ownFilename.back());
                return -1;
            }
            
            bytecode.push_back(0x02);
            keyword = tokens[0];
            auto var_index = resolveVariableIndex(keyword, compilerData);
            varIndex_assign = var_index;
            bytecode.push_back(var_index);
            op = NONE;
            break;
        }
        else if (token == "label") {
            if (op != NONE) {
                printError("Syntax error", state.lineIndex, state.ownFilename.back());
                return -1;
            }
            op = LABEL;
            continue;
        }
        else if (token == "jump") {
            if (op != NONE) {
                printError("Syntax error", state.lineIndex, state.ownFilename.back());
                return -1;
            }
            op = JUMP;
            continue;
        }
        else if (token == "if") {
            if (op != NONE) {
                printError("Syntax error", state.lineIndex, state.ownFilename.back());
                return -1;
            }
            op = IF;
            state.blockDepth.push_back(BlockType::IF);
            state.elseDefined.push_back(false);
            state.condJumpStack.push_back(-1);
            state.elseJumpStack.push_back({-1});
            continue;
        }
        else if (token == "endif") {
            if (state.blockDepth.size() == 0 || state.blockDepth.back() != BlockType::IF) {
                printError("Unexpected 'endif' (no matching 'if')", state.lineIndex, state.ownFilename.back());
                return -1;
            }
            bool isElseDefined = state.elseDefined.back();
            int i;
            for (i = state.blockDepth.size() - 1; i > 0; i--) {
                if (state.blockDepth[i] == BlockType::IF) break;
            }
            if (isElseDefined) {
                auto toPatch = state.elseJumpStack.back();
                for (auto loc : toPatch) {
                    if (loc == -1) continue;
                    patchUint32(bytecode, loc, static_cast<uint32_t>(bytecode.size()));
                }
            }
            else {
                int loc = state.condJumpStack.back();
                patchUint32(bytecode, loc, static_cast<uint32_t>(bytecode.size()));
            }
            state.condJumpStack.pop_back();
            state.elseJumpStack.pop_back();
            state.blockDepth.erase(state.blockDepth.begin() + i);
            state.elseDefined.pop_back();
            continue;
        }
        else if (token == "else") {
            if (state.blockDepth.size() == 0 || state.blockDepth.back() != BlockType::IF) {
                printError("Unexpected 'else' (no matching 'if')", state.lineIndex, state.ownFilename.back());
                return -1;
            }
            state.elseDefined.back() = true;
            int loc = state.condJumpStack.back();

            // Patch false-jump location to skip the upcoming 5-byte 'JUMP32 target' instruction (1 byte opcode + 4 bytes uint32)
            patchUint32(bytecode, loc, static_cast<uint32_t>(bytecode.size() + 5));

            bytecode.push_back(0x06); // JUMP32
            emitUint32(bytecode, 0x00000000);
            state.elseJumpStack.back().back() = static_cast<int>(bytecode.size() - 4); // Track location of jump target
            continue;
        } else if(token == "elif") {
            // else
            if (state.blockDepth.size() == 0 || state.blockDepth.back() != BlockType::IF) {
                printError("Unexpected 'elif' (no matching 'if')", state.lineIndex, state.ownFilename.back());
                return -1;
            }
            state.elseDefined.back() = true;
            int loc = state.condJumpStack.back();

            // Patch false-jump location to skip the upcoming 5-byte 'JUMP32 target' instruction (1 byte opcode + 4 bytes uint32)
            patchUint32(bytecode, loc, static_cast<uint32_t>(bytecode.size() + 5));

            bytecode.push_back(0x06); // JUMP32
            emitUint32(bytecode, 0x00000000);
            state.elseJumpStack.back().back() = static_cast<int>(bytecode.size() - 4); // Track location of jump target

            // if
            state.condJumpStack.push_back(-1);
            state.elseJumpStack.back().push_back(-1);
            op = IF;

            continue;
        }
        else if (token == "while") {
            if (op != NONE) {
                printError("Syntax error", state.lineIndex, state.ownFilename.back());
                return -1;
            }
            op = WHILE;
            state.loopDepth++;
            state.blockDepth.push_back(BlockType::WHILE);
            state.loopCondJumpStack.push_back(LoopJumpData{ static_cast<int>(bytecode.size()), {} });
            continue;
        }
        else if (token == "endwhile") {
            if (state.blockDepth.size() == 0 || state.blockDepth.back() != BlockType::WHILE) {
                printError("Unexpected 'endwhile' (no matching 'while')", state.lineIndex, state.ownFilename.back());
                return -1;
            }
            state.loopDepth--;
            auto loopData = state.loopCondJumpStack.back();
            bytecode.push_back(0x06);
            emitUint32(bytecode, static_cast<uint32_t>(loopData.locStart));
            for (auto& loc : loopData.unresolvedEnds) {
                patchUint32(bytecode, loc, static_cast<uint32_t>(bytecode.size()));
            }
            state.loopCondJumpStack.pop_back();
            state.blockDepth.pop_back();
            continue;
        }
        else if (token == "repeat") {
            if (op != NONE) {
                printError("Syntax error", state.lineIndex, state.ownFilename.back());
                return -1;
            }
            op = REPEAT;
            state.loopDepth++;
            state.blockDepth.push_back(BlockType::REPEAT);
            continue;
        }
        else if (token == "endrepeat") {
            if (state.blockDepth.size() == 0 || state.blockDepth.back() != BlockType::REPEAT) {
                printError("Unexpected 'endrepeat' (no matching 'repeat')", state.lineIndex, state.ownFilename.back());
                return -1;
            }
            state.loopDepth--;

            auto loopData = state.loopCondJumpStack.back();
            bytecode.push_back(0x06);
            emitUint32(bytecode, static_cast<uint32_t>(loopData.locStart));
            for (auto& loc : loopData.unresolvedEnds) {
                patchUint32(bytecode, loc, static_cast<uint32_t>(bytecode.size()));
            }
            state.loopCondJumpStack.pop_back();
            state.blockDepth.pop_back();
            continue;
        }
        else if (token == "halt") {
            if (op != NONE) {
                printError("Syntax error", state.lineIndex, state.ownFilename.back());
                return -1;
            }
            bytecode.push_back(0xFF);
            
            continue;
        }
        else if (token == "continue") {
            if (state.loopDepth <= 0) {
                printError("Unexpected 'continue' (outside loop body)", state.lineIndex, state.ownFilename.back());
                return -1;
            }

            auto loopData = state.loopCondJumpStack.back();
            bytecode.push_back(0x06);
            emitUint32(bytecode, static_cast<uint32_t>(loopData.locStart));
        }
        else if (token == "break") {
            if (state.loopDepth <= 0) {
                printError("Unexpected 'break' (outside loop body)", state.lineIndex, state.ownFilename.back());
                return -1;
            }

            bytecode.push_back(0x06); // jump
            int loc = static_cast<int>(bytecode.size()); // location for patch
            emitUint32(bytecode, 0x00000000); // jump offset
            state.loopCondJumpStack.back().unresolvedEnds.push_back(loc);
        }
        else if (token == "routine") {
            if (op != NONE) {
                printError("Syntax error", state.lineIndex, state.ownFilename.back());
                return -1;
            }
            op = SUBROUTINE;
            if (state.inRoutine) {
                printError("Nested routines/functions are not allowed", state.lineIndex, state.ownFilename.back());
                return -1;
            }
            state.inRoutine = true;
            continue;
        }
        else if (token == "endroutine") {
            if (op != NONE) {
                printError("Syntax error", state.lineIndex, state.ownFilename.back());
                return -1;
            }
            if (!state.inRoutine) {
                printError("Unexpected 'endroutine' (no matching 'routine')", state.lineIndex, state.ownFilename.back());
                return -1;
            }
            bytecode.push_back(0xFE); // RET
            state.inRoutine = false;
            state.routineIndex = -1;
            continue;
        }
        else if (token == "function") {
            if (op != NONE) {
                printError("Syntax error", state.lineIndex, state.ownFilename.back());
                return -1;
            }
            op = SUBROUTINE;
            if (state.inFunction) {
                printError("Nested routines/functions are not allowed", state.lineIndex, state.ownFilename.back());
                return -1;
            }
            state.inFunction = true;
            continue;
        }
        else if (token == "endfunction") {
            if (op != NONE) {
                printError("Syntax error", state.lineIndex, state.ownFilename.back());
                return -1;
            }
            if (!state.inFunction) {
                printError("Unexpected 'endfunction' (no matching 'function')", state.lineIndex, state.ownFilename.back());
                return -1;
            }
            bytecode.push_back(0xFE); // RET
            state.inFunction = false;
            state.routineIndex = -1;
            continue;
        }
        else if (token == "return") {
            if (op != NONE) {
                printError("Syntax error", state.lineIndex, state.ownFilename.back());
                return -1;
            }
            if (!state.inFunction) {
                printError("'return' is not allowed outside routine block", state.lineIndex, state.ownFilename.back());
                return -1;
            }
            if(tokens.size() < 2) {
                printError("Syntax error: expected statement after 'return'", state.lineIndex, state.ownFilename.back());
                return -1;
            }
            // assemble expression
            std::string statement;
            for(int i = 1; i < tokens.size(); i++) {
                statement += tokens[i] + " ";
            }
            // compile expression
            try {
                compileExpression(
                    statement, compilerData, bytecode
                ); // result in stack
            } catch (const std::exception& e) {
                printError(e.what(), state.lineIndex, state.ownFilename.back());
                return -1;
            }
            // result remains in stack to be consumed
            // emit RET
            bytecode.push_back(0xFE); // RET
            break;
        }
        else if(token == "import") 
        {
            // check syntax first
            if(tokens.size() != 2 || op != NONE) {
                printError("Syntax error", state.lineIndex, state.ownFilename.back());
                return -1;
            }

            // check depth
            if(state.importDepth > 4) {
                printError("Reached maximum import depth of 5", state.lineIndex, state.ownFilename.back());
                return -1;
            }

            // get file name to import
            std::string file = tokens[1];
            replaceAll(file, "'", "");
            replaceAll(file, "\"", "");

            // check edge cases
            if(file == state.ownFilename.back()) {
                printError("import error: self-import", state.lineIndex, state.ownFilename.back());
                return -1;
            }

            // recursively compile next script
            state.importDepth++;
            std::ifstream fileStream(file);
            int status = compileFromFile(fileStream, compilerData, verbose, debugInfo, file, &state);
            state.importDepth--;
            state.ownFilename.pop_back();
            if(status != 0) {
                printError("Script compilation has failed", state.lineIndex, state.ownFilename.back());
                return -1;
            }

            // bytecode is already emitted into main part
            break;
        }
        else {
            if (op == NONE) {
                if (tokens.size() > 1 && tokens[1] == "=") {
                    continue;
                }
                auto it = funcList.find(token);
                if (it != funcList.end()) {
                    op = FUNC_CALL;
                    funcIndex = it->second.opcode;
                    state.funcArgs = 0;
                    state.requiredFuncArgs = it->second.argCount;
                } else {
                    op = ROUTINE_CALL;

                    routineToCall = tokens[0];
                    state.funcArgs = 0;
                    auto sigIt = compilerData->routineList.find(routineToCall);
                    state.requiredFuncArgs = (sigIt != compilerData->routineList.end()) ? sigIt->second.argCount : 0;
                    int currRoutine = !isMainBody ? state.routineIndex : -1;
                    state.unresolvedRoutineCalls.push_back({ routineToCall, 0, state.lineIndex, currRoutine, 0 });
                } 
                continue;
            }
        }

        keyword = token;

        switch (op) {
        case SUBROUTINE:
        {
            if (state.routineIndex == -1) {
                auto sigIt = compilerData->routineList.find(token);
                state.routineIndex = (sigIt != compilerData->routineList.end())
                    ? sigIt->second.index
                    : state.routineCount++; // shouldn't happen; prescan covers every 'routine' line
                if (state.routineIndex >= state.routineCount) state.routineCount = state.routineIndex + 1;
                compilerData->currentRoutineIndex = state.routineIndex;
                state.subroutineIndexMap[token] = state.routineIndex;
                state.subroutineBytecode[state.routineIndex] = std::vector<uint8_t>();
                state.subroutineInfoMap[state.routineIndex] = SubroutineInfo {{}};
            }
        }
        [[fallthrough]];
        case FUNC_CALL:
        case ROUTINE_CALL:
        {
            if(token == "(") {
                if(argsOpen) {
                    callParenDepth++;
                    state.functionArgument += token + " ";
                    break;
                }
                state.functionArgument.clear();
                argsOpen = true;
                callParenDepth = 0;
                break;
            } else if (token == ")") {
                if(!argsOpen) {
                    // error
                    printError("Unexpected \')\'", state.lineIndex, state.ownFilename.back());
                    return -1;
                }
                if(callParenDepth > 0) {
                    callParenDepth--;
                    state.functionArgument += token + " ";
                    break;
                }
                argsOpen = false;
                // arguments end
            } else if (token == ",") {
                if(argsOpen && callParenDepth == 0) {
                    if(op == SUBROUTINE) {
                        state.subroutineInfoMap[state.routineIndex].args.push_back(state.functionArgument);
                    } else {
                        try {
                            compileExpression(
                                state.functionArgument, compilerData, bytecode
                            ); // result in stack
                        } catch (const std::exception& e) {
                            printError(e.what(), state.lineIndex, state.ownFilename.back());
                            return -1;
                        }

                        if(op == ROUTINE_CALL) {
                            state.unresolvedRoutineCalls.back().argCount++;
                        }
                    }
                    state.funcArgs++;
                    state.functionArgument.clear();
                    break;
                }
            }
            if(argsOpen) {
                state.functionArgument += token + " ";
            }
        }
        break;
        case LABEL:
        {
            if (!isMainBody) {
                state.routineLabels[state.routineIndex][keyword] = static_cast<int>(bytecode.size());
            }
            else {
                state.globalLabels[keyword] = static_cast<int>(bytecode.size());
            }
            op = NONE;
        }
        break;
        case JUMP:
        {
            int currentCtx = !isMainBody ? state.routineIndex : -1;
            bool found = false;
            int targetOffset = 0;

            if (!isMainBody) {
                auto& rMap = state.routineLabels[state.routineIndex];
                auto it = rMap.find(keyword);
                if (it != rMap.end()) {
                    found = true;
                    targetOffset = it->second;
                }
            }
            else {
                auto it = state.globalLabels.find(keyword);
                if (it != state.globalLabels.end()) {
                    found = true;
                    targetOffset = it->second;
                }
            }

            bytecode.push_back(0x06); // JUMP32
            int loc = static_cast<int>(bytecode.size());

            if (found) {
                emitUint32(bytecode, static_cast<uint32_t>(targetOffset));
            }
            else {
                emitUint32(bytecode, 0x00000000);
                state.unresolvedJumps.push_back({ keyword, loc, state.lineIndex, currentCtx });
            }
            op = NONE;
        }
        break;
        case REPEAT:
        {
            if (token == ",") continue; // skip delimiter char
            if(conditionArgs > 2) {
                printError("Too much arguments", state.lineIndex, state.ownFilename.back());
                return -1;
            }
            tokenStack.push_back(token);
            conditionArgs++;
        }
        break;
        case WHILE:
        case IF:
        {
            auto it = condOpMap.find(keyword);
            if (it != condOpMap.end()) {
                if (condOp != COP_NONE || state.conditionTokens.size() == 0) {
                    printError("Syntax error", state.lineIndex, state.ownFilename.back());
                    return -1;
                } 

                // compile expression
                try {
                    compileExpression(
                        state.conditionTokens, compilerData, bytecode
                    ); // result in stack
                    state.conditionTokens.clear();
                }
                catch (const std::exception& e) {
                    printError(e.what(), state.lineIndex, state.ownFilename.back());
                    return -1;
                }

                conditionArgs++;

                condOp = it->second;
            }
            else {
                if (conditionArgs > 1 && condOp != COP_NONE) {
                    printError("Syntax error", state.lineIndex, state.ownFilename.back());
                    return -1;
                }
                state.conditionTokens += token + " ";
            }
        }
        break;
        default:
            break;
        }
    }

    switch (op) {
    case FUNC_CALL:
    case ROUTINE_CALL:
    case SUBROUTINE:
        if(!state.functionArgument.empty()) {
            if(op == SUBROUTINE) {
                state.subroutineInfoMap[state.routineIndex].args.push_back(state.functionArgument);
            } else {
                try {
                    compileExpression(
                        state.functionArgument, compilerData, bytecode
                    ); // result in stack
                } catch (const std::exception& e) {
                    printError(e.what(), state.lineIndex, state.ownFilename.back());
                    return -1;
                }

                if(op == ROUTINE_CALL) {
                    state.unresolvedRoutineCalls.back().argCount++;
                }
            }
            state.funcArgs++;
            state.functionArgument.clear();
        }

        if(op != SUBROUTINE) {
            if(state.funcArgs != state.requiredFuncArgs) {
                std::string key;
                if(op == FUNC_CALL) {
                    auto it = std::find_if(funcList.begin(), funcList.end(),
                        [&](const auto& p) { return p.second.opcode == funcIndex; });
                    key = (it != funcList.end()) ? it->first : "<unknown>";
                } else {
                    key = routineToCall;
                }
                std::stringstream ss;
                ss << "argument count mismatch for '" << key << "': expected " 
                    << state.requiredFuncArgs << ", got " << state.funcArgs << "\n";
                printError(ss.str(), state.lineIndex, state.ownFilename.back());
                return -1;
            }
            if(op == FUNC_CALL) {
                bytecode.push_back(0x04); // call function
                bytecode.push_back(funcIndex);
            }
            else if(op == ROUTINE_CALL) {
                bytecode.push_back(0x07); // CALL32
                int loc = static_cast<int>(bytecode.size());
                emitUint32(bytecode, 0x00000000); // 4-byte placeholder

                state.unresolvedRoutineCalls.back().location = loc;
            }
        } else {
            // POP args into variables
            auto& args = state.subroutineInfoMap[state.routineIndex].args;
            for(int i = static_cast<int>(args.size()) - 1; i >= 0; i--) {
                state.subroutineBytecode[state.routineIndex].push_back(0x02);

                auto var_index = resolveVariableIndex(args[i], compilerData);
                state.subroutineBytecode[state.routineIndex].push_back(var_index);
            }
        }
        break;
    case WHILE:
    case IF:
    {
        if (state.conditionTokens.size() == 0) {
            printError("Syntax error", state.lineIndex, state.ownFilename.back());
            return -1;
        }

        // compile expression
        try {
            compileExpression(
                state.conditionTokens, compilerData, bytecode
            ); // result in stack
            state.conditionTokens.clear();
        }
        catch (const std::exception& e) {
            printError(e.what(), state.lineIndex, state.ownFilename.back());
            return -1;
        }
        conditionArgs++;

        auto it = condOpcodeMap.find(condOp);
        if (it != condOpcodeMap.end()) {
            if (conditionArgs != 2) {
                printError("Syntax error", state.lineIndex, state.ownFilename.back());
                return -1;
            }
            bytecode.push_back(it->second); // IF32 opcode
            int loc = static_cast<int>(bytecode.size());
            emitUint32(bytecode, 0x00000000);
            switch (op) {
            case IF:
                state.condJumpStack.back() = loc;
                break;
            case WHILE:
                state.loopCondJumpStack.back().unresolvedEnds.push_back(loc);
                break;
            }
        }
        else {
            printError("Syntax error", state.lineIndex, state.ownFilename.back());
            return -1;
        }
        break;
    }
    case REPEAT: {
        // init var
        auto variable = "cnt_" + std::to_string(state.blockDepth.size());
        pushToStack("0", compilerData, bytecode);
        bytecode.push_back(0x02); // POP
        auto idx = resolveVariableIndex(variable, compilerData);
        bytecode.push_back(idx); // to other variable

        // mark loop start
        state.loopCondJumpStack.push_back(LoopJumpData{ static_cast<int>(bytecode.size()), {} });

        // if x < n
        pushToStack(variable, compilerData, bytecode); // x
        if (conditionArgs == 2) {
            // iterator var name arg
            auto var_index = resolveVariableIndex(tokenStack[1], compilerData);
            // copy to user specified iterator variable
            bytecode.push_back(0xAB);
            bytecode.push_back(var_index);
        } 
        bytecode.push_back(0xA8); // INCV
        bytecode.push_back(idx);
        pushToStack(tokenStack[0], compilerData, bytecode); // n
        bytecode.push_back(0xC2); // x < n
        int loc = static_cast<int>(bytecode.size()); // location for patch
        emitUint32(bytecode, 0x00000000); // jump offset
        state.loopCondJumpStack.back().unresolvedEnds.push_back(loc);
        break;
    }
    }
    op = NONE;

    tokenStack.clear();
    state.lineIndex++;

    return 0;
}