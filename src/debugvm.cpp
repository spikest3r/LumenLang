#include "vm.h"
#include "disassembler.h"
#include "tokenizer.h"

void begin_execution(
    const std::vector<int>& bytecode,
    const std::vector<std::string>& stringPool,
    std::vector<Variant>& variables,
    std::vector<Variant>& stack,
    std::vector<int>& pcStack,
    int& PC,
    bool& halt,
    const std::unordered_set<int>& breakpoints
);

void step(
    const std::vector<int>& bytecode,
    const std::vector<std::string>& stringPool,
    std::vector<Variant>& variables,
    std::vector<Variant>& stack,
    std::vector<int>& pcStack,
    int& PC,
    bool& halt
);

void printHelp();

bool askYesNo(const std::string& prompt, bool defaultVal = false);

int run_debug(
    const std::string& filename,
    const std::vector<int>& bytecode,
    const std::vector<std::string>& stringPool,
    const int& variableIndex
) {
    std::cout << "Debugger active. 'help' for list of commands" << std::endl;

    // vm
    std::vector<Variant> variables;
    variables.resize(variableIndex);

    std::vector<Variant> stack;
    std::vector<int> pcStack;

    int PC = 0;
    bool halt = false;

    // debugger
    std::unordered_set<int> breakpoints;
    bool debugSymbolsSpecified = false;
    std::string debugSymbolsFile;
    bool debugSymbolsValid = false;

    std::unordered_map<int, std::string> debugVariablesMap;
    std::unordered_map<std::string, RoutineInfo> debugRoutinesMap;
    std::unordered_map<int, std::string> debugFuncListMap;

    bool resume = false;

    bool newLine = true;

    while(true) {
        if(newLine) std::cout << ">> ";
        newLine = true;
        std::string input;
        std::getline(std::cin, input);

        auto tokens = tokenizeFormula(input);
        if(tokens.size() < 1) {
            newLine = false;
            continue;
        }
        auto command = tokens[0];

        if(command == "help") {
            printHelp();
        } else if(command == "run") {
            if(resume) {
                bool result = askYesNo("This will restart execution. Are you sure?", false);
                if(!result) continue;
            }

            std::cout << "Executing..." << std::endl;

            PC = 0;
            halt = false;
            variables.clear();
            stack.clear();
            pcStack.clear();
            variables.resize(variableIndex);

            begin_execution(
                bytecode, stringPool,
                variables, stack, pcStack,
                PC, halt,
                breakpoints
            );

            resume = !halt;

            if(halt) std::cout << "Execution finished" << std::endl;
        } else if(command == "stop") {
            if(!resume) {
                std::cout << "Execution has not started. Use 'run' first." << std::endl;
            } else {
                resume = false;
                std::cout << "Execution finished" << std::endl;
            }
        } else if(command == "breakpoint") {
            auto arg0 = tokens[1];
            if(arg0 == "set") {
                if(tokens.size() != 3) {
                    std::cout << "Invalid argument count" << std::endl;
                    continue;
                }
                int value = std::stoi(tokens[2], nullptr, 16);
                breakpoints.insert(value);
                std::cout << "Breakpoint set at " << tokens[2] << std::endl;
            } else if(arg0 == "remove") {
                if(tokens.size() != 3) {
                    std::cout << "Invalid argument count" << std::endl;
                    continue;
                }
                int value = std::stoi(tokens[2], nullptr, 16);
                breakpoints.erase(value);
                std::cout << "Breakpoint at " << tokens[2] << " removed" << std::endl;
            } else if(arg0 == "list") {
                if(tokens.size() != 2) {
                    std::cout << "Invalid argument count" << std::endl;
                    continue;
                }
                for(const int& it: breakpoints) {
                    std::cout << "0x" << std::hex << std::right << std::setw(8) << std::setfill('0') << it << std::endl;
                }
                std::cout << std::dec;
            } else if(arg0 == "clear") {
                if(tokens.size() != 2) {
                    std::cout << "Invalid argument count" << std::endl;
                    continue;
                }
                bool result = askYesNo("This will clear all breakpoints. Are you sure?", false);
                if(!result) continue;
                breakpoints.clear();
            } else {
                std::cout << "Invalid arguments" << std::endl;
            }
        } else if(command == "pc") {
            if(tokens.size() == 1) {
                // get pc
                std::cout << "PC = " << "0x" << std::hex << std::right << std::setw(8) << std::setfill('0') << PC << std::endl;
                std::cout << std::dec;
            } else if(tokens.size() == 2) {
                // set pc
                if(!resume) {
                    std::cout << "Execution has not started. Use 'run' first." << std::endl;
                } else {
                    int value = std::stoi(tokens[1], nullptr, 16);
                    PC = value;
                    std::cout << "PC = " << "0x" << std::hex << std::right << std::setw(8) << std::setfill('0') << PC << std::endl;
                    std::cout << std::dec;
                }
            }
        } else if(command == "step" || command == "s") {
            if(!resume) {
                std::cout << "Execution has not started. Use 'run' first." << std::endl;
            } else {
                step(
                    bytecode, stringPool,
                    variables, stack, pcStack,
                    PC, halt
                );
            }
        } else if(command == "continue") {
            if(!resume) {
                std::cout << "Execution has not started. Use 'run' first." << std::endl;
            } else {
                step(
                    bytecode, stringPool,
                    variables, stack, pcStack,
                    PC, halt
                );

                begin_execution(
                    bytecode, stringPool,
                    variables, stack, pcStack,
                    PC, halt,
                    breakpoints
                );

                resume = !halt;

                if(halt) std::cout << "Execution finished" << std::endl;
            }
        } else if(command == "stack") {
            if(!resume) {
                std::cout << "Execution has not started. Use 'run' first." << std::endl;
            } else {
                std::cout << "Stack (top to bottom):\n";
                if (stack.empty()) {
                    std::cout << "  (empty)\n";
                }
                for (size_t i = stack.size(); i-- > 0; ) {
                    std::cout << "  [" << (stack.size() - 1 - i) << "] "
                            << variantToString(stack[i]);
                    if (i == stack.size() - 1) std::cout << "  <- top";
                    std::cout << "\n";
                }
            }
        } else if(command == "variables") {
            if(!resume) {
                std::cout << "Execution has not started. Use 'run' first." << std::endl;
            } else {
                std::cout << "Variables:\n";
                if (variables.empty()) {
                    std::cout << "  (empty)\n";
                }
                for (size_t i = 0; i < variables.size(); i++) {
                    auto varName = debugSymbolsValid ? debugVariablesMap[i] : std::to_string(i);
                    std::cout << "  [" << varName << "] "
                            << variantToString(variables[i]);
                    std::cout << "\n";
                }
            }
        }
        else if(command == "disassemble") {
            disassemble(
                bytecode, stringPool, 
                debugSymbolsSpecified ? debugSymbolsFile : "",
                nullptr, resume ? PC : -1
            );
        } else if(command == "debugsymbols") {
            std::cout << "Please specify .dbg file for this binary" << std::endl;
            std::cout << ": ";
            std::cin >> debugSymbolsFile;
            debugSymbolsSpecified = true;

            debugVariablesMap.clear();
            debugRoutinesMap.clear();
            debugFuncListMap.clear();

            debugSymbolsValid = loadDebugInfo(debugSymbolsFile,
                debugVariablesMap,
                debugRoutinesMap,
                debugFuncListMap
            );
        }
        else {
            std::cout << "Unknown command" << std::endl;
        }
    }
}

void begin_execution(
    const std::vector<int>& bytecode,
    const std::vector<std::string>& stringPool,
    std::vector<Variant>& variables,
    std::vector<Variant>& stack,
    std::vector<int>& pcStack,
    int& PC,
    bool& halt,
    const std::unordered_set<int>& breakpoints
) {
    while(true) {
        if(breakpoints.count(PC)) {
            std::cout << "Breakpoint hit!" << std::endl;
            return;
        }
        
        step(
            bytecode, stringPool,
            variables, stack, pcStack,
            PC, halt
        );

        if(halt) break;
    }
}

void step(
    const std::vector<int>& bytecode,
    const std::vector<std::string>& stringPool,
    std::vector<Variant>& variables,
    std::vector<Variant>& stack,
    std::vector<int>& pcStack,
    int& PC,
    bool& halt
) {
    auto opcode = bytecode[PC];
    int offset = getOpCodeOffset(opcode);

    int result = execute(bytecode, stringPool, variables, stack, pcStack, PC, halt);

    PC = result;
}

void printHelp() {
    std::cout << "run - Begin bytecode execution" << std::endl;
    std::cout << "stop - Stop execution" << std::endl;
    std::cout << "breakpoint [set, remove, list, clear] [address] - Set, remove, list or clear breakpoints" << std::endl;
    std::cout << "pc - Get PC value" << std::endl;
    std::cout << "pc [address] - Set PC value" << std::endl;
    std::cout << "step - Step one instruction" << std::endl;
    std::cout << "continue - Continue execution" << std::endl;
    std::cout << "stack - Show stack contents" << std::endl;
    std::cout << "variables - Show all variables and their content" << std::endl;
    std::cout << "disassemble - View full disassembly" << std::endl;
    std::cout << "debugsymbols - Specify debug symbols file" << std::endl;
}

bool askYesNo(const std::string& prompt, bool defaultVal) {
    std::string input;
    while (true) {
        std::cout << prompt << (defaultVal ? " [Y/n] " : " [y/N] ");

        if (!std::getline(std::cin, input)) {
            return defaultVal; // EOF/stream closed
        }

        size_t start = input.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
            return defaultVal; // blank Enter -> default
        }
        size_t end = input.find_last_not_of(" \t\r\n");
        input = input.substr(start, end - start + 1);

        if (input.size() == 1) {
            char c = std::tolower(static_cast<unsigned char>(input[0]));
            if (c == 'y') return true;
            if (c == 'n') return false;
        }

        std::cout << "Invalid option\n";
    }
}