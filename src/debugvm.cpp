#include "vm.h"
#include "disassembler.h"
#include "tokenizer.h"

void begin_execution(
    VMProgramData* progData,
    VMExecutionData* execData,
    const std::unordered_set<int>& breakpoints
);

void step(
    VMProgramData* progData,
    VMExecutionData* execData
);

void printHelp();
bool askYesNo(const std::string& prompt, bool defaultVal = false);

void zeroExecData(VMExecutionData* execData, VMProgramData* progData);

int run_debug(
    VMProgramData* progData
) {
    std::cout << "Debugger active. 'help' for list of commands" << std::endl;

    // vm
    VMExecutionData execData;

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

            zeroExecData(&execData, progData);

            begin_execution(
                progData, &execData, breakpoints
            );

            resume = !execData.halt;

            if(execData.halt) std::cout << "Execution finished" << std::endl;
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
                std::cout << "PC = " << "0x" << std::hex << std::right << std::setw(8) << std::setfill('0') << execData.PC << std::endl;
                std::cout << std::dec;
            } else if(tokens.size() == 2) {
                // set pc
                if(!resume) {
                    std::cout << "Execution has not started. Use 'run' first." << std::endl;
                } else {
                    int value = std::stoi(tokens[1], nullptr, 16);
                    execData.PC = value;
                    std::cout << "PC = " << "0x" << std::hex << std::right << std::setw(8) << std::setfill('0') << execData.PC << std::endl;
                    std::cout << std::dec;
                }
            }
        } else if(command == "step" || command == "s") {
            if(!resume) {
                std::cout << "Execution has not started. Use 'run' first." << std::endl;
            } else {
                step(
                    progData, &execData
                );
            }
        } else if(command == "continue") {
            if(!resume) {
                std::cout << "Execution has not started. Use 'run' first." << std::endl;
            } else {
                step(
                    progData, &execData
                );

                begin_execution(
                    progData, &execData, breakpoints
                );

                resume = !execData.halt;

                if(execData.halt) std::cout << "Execution finished" << std::endl;
            }
        } else if(command == "stack") {
            if(!resume) {
                std::cout << "Execution has not started. Use 'run' first." << std::endl;
            } else {
                std::cout << "Stack (top to bottom):\n";
                if (execData.stack.empty()) {
                    std::cout << "  (empty)\n";
                }
                for (size_t i = execData.stack.size(); i-- > 0; ) {
                    std::cout << "  [" << (execData.stack.size() - 1 - i) << "] "
                            << variantToString(execData.stack[i]);
                    if (i == execData.stack.size() - 1) std::cout << "  <- top";
                    std::cout << "\n";
                }
            }
        } else if(command == "variables") {
            if(!resume) {
                std::cout << "Execution has not started. Use 'run' first." << std::endl;
            } else {
                std::cout << "Variables:\n";
                if (execData.variables.empty()) {
                    std::cout << "  (empty)\n";
                }
                for (size_t i = 0; i < execData.variables.size(); i++) {
                    auto varName = debugSymbolsValid ? debugVariablesMap[i] : std::to_string(i);
                    std::cout << "  [" << varName << "] "
                            << variantToString(execData.variables[i]);
                    std::cout << "\n";
                }
            }
        }
        else if(command == "disassemble") {
            disassemble(
                progData->bytecode, progData->stringPool, progData->constPool,
                debugSymbolsSpecified ? debugSymbolsFile : "",
                nullptr, resume ? execData.PC : -1
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
        } else if(command == "quit" || command == "exit") {
            bool confirm = true;
            if(resume) {
                confirm = askYesNo("Script is still running. Proceed?");
            }
            if(confirm) exit(0);
        }
        else {
            std::cout << "Unknown command" << std::endl;
        }
    }
}

void begin_execution(
    VMProgramData* progData,
    VMExecutionData* execData,
    const std::unordered_set<int>& breakpoints
) {
    while(true) {
        if(breakpoints.count(execData->PC)) {
            std::cout << "Breakpoint hit!" << std::endl;
            return;
        }
        
        step(
            progData, execData
        );

        if(execData->halt) break;
    }
}

void step(
    VMProgramData* progData,
    VMExecutionData* execData
) {
    auto opcode = progData->bytecode[execData->PC];
    int offset = getOpCodeOffset(opcode);

    execData->PC = execute(progData, execData);
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

void zeroExecData(VMExecutionData* execData, VMProgramData* progData) {
    execData->PC = 0;
    execData->halt = false;

    execData->variables.clear();
    execData->stack.clear();
    execData->pcStack.clear();

    execData->variables.clear();
    execData->variables.resize(progData->variableCount);
}