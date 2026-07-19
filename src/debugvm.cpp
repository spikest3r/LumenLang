#include "vm.h"
#include "disassembler.h"
#include "tokenizer.h"
#include <chrono>
#include <map>
#include <algorithm>
#include <sstream>
#include <iomanip>

static std::string formatDuration(double ms)
{
    std::ostringstream ss;
    ss << std::fixed;

    if (ms >= 1.0)
    {
        ss << std::setprecision(3) << ms << " ms";
    }
    else if (ms >= 0.001)
    {
        ss << std::setprecision(3) << ms * 1000.0 << " us";
    }
    else
    {
        ss << std::setprecision(0) << ms * 1'000'000.0 << " ns";
    }

    return ss.str();
}

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

static constexpr double BLOCKING_SAMPLE_THRESHOLD_MS = 50.0;

struct OpcodeProfile {
    long long count = 0;
    double totalMs = 0.0;
    double minMs = -1.0;
    double maxMs = 0.0;
    long long blockingCount = 0;   // samples excluded as likely I/O waits
    double blockingTotalMs = 0.0;  // wall time spent in those samples

    void addSample(double ms) {
        if (ms > BLOCKING_SAMPLE_THRESHOLD_MS) {
            blockingCount++;
            blockingTotalMs += ms;
            return;
        }
        count++;
        totalMs += ms;
        if (minMs < 0.0 || ms < minMs) minMs = ms;
        if (ms > maxMs) maxMs = ms;
    }
};

struct ProfilingSession {
    bool enabled = false;

    std::map<int, OpcodeProfile> byOpcode;
    long long totalInstructions = 0;      // includes blocking samples
    double totalTimedMs = 0.0;            // excludes blocking samples
    double totalBlockingMs = 0.0;         // time spent presumed-blocked
    std::chrono::steady_clock::time_point runStart;
    bool running = false;

    void reset() {
        byOpcode.clear();
        totalInstructions = 0;
        totalTimedMs = 0.0;
        totalBlockingMs = 0.0;
        running = false;
    }

    void beginRun() {
        runStart = std::chrono::steady_clock::now();
        running = true;
    }

    void recordInstruction(int opcode, double ms) {
        totalInstructions++;
        auto& prof = byOpcode[opcode];
        prof.addSample(ms);
        if (ms > BLOCKING_SAMPLE_THRESHOLD_MS) {
            totalBlockingMs += ms;
        } else {
            totalTimedMs += ms;
        }
    }

    double wallElapsedMs() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<double, std::milli>(now - runStart).count();
    }
};

static ProfilingSession g_profiler;

static std::string opcodeName(int opcode) {
    // TODO: replace with real mnemonic lookup
    auto it = disassemblyMap.find(opcode);
    if(it != disassemblyMap.end()) {
        return it->second;
    } else {
        return "XXX";
    }
}

static void printProfilingReport() {
    if (g_profiler.totalInstructions == 0) {
        std::cout << "No instructions were profiled." << std::endl;
        return;
    }

    std::cout << "\n===== Profiling Report =====\n";
    std::cout << "Total instructions executed: " << g_profiler.totalInstructions << "\n";
    std::cout << "Total execution time (excl. blocking waits): "
          << formatDuration(g_profiler.totalTimedMs) << "\n";
    if (g_profiler.totalBlockingMs > 0.0) {
        std::cout << "Total time in likely blocking/input instructions: "
                   << g_profiler.totalBlockingMs << " ms"
                   << " (threshold: " << BLOCKING_SAMPLE_THRESHOLD_MS << " ms/instr)\n";
    }
    double wallMs = g_profiler.totalTimedMs + g_profiler.totalBlockingMs;
    std::cout << "Total wall time: "
          << formatDuration(wallMs) << "\n";
    if (g_profiler.totalTimedMs > 0.0 && g_profiler.totalInstructions > 0) {
        std::cout << "Average time/instruction (excl. blocking): "
                   << (g_profiler.totalTimedMs /
                       (g_profiler.totalInstructions))
                   << " ms\n";
    }

    if (g_profiler.totalTimedMs > 0.0)
    {
        double seconds = g_profiler.totalTimedMs / 1000.0;

        double ips  = g_profiler.totalInstructions / seconds;
        double mips = ips / 1'000'000.0;

        // Instructions that were actually timed (non-blocking)
        long long effectiveInstructions = 0;
        for (const auto& [_, prof] : g_profiler.byOpcode)
            effectiveInstructions += prof.count;

        double effectiveIPC =
            static_cast<double>(effectiveInstructions) /
            g_profiler.totalInstructions;

        std::cout << std::fixed;

        std::cout << "Instructions/sec: "
                << std::setprecision(0)
                << ips << "\n";

        std::cout << "MIPS: "
                << std::setprecision(3)
                << mips << "\n";

        std::cout << "Timed instruction ratio: "
                << std::setprecision(3)
                << effectiveIPC * 100.0
                << "% ("
                << effectiveInstructions
                << "/"
                << g_profiler.totalInstructions
                << " instructions timed)\n";

        std::cout << "Average execution rate: "
                << std::setprecision(3)
                << mips
                << " MIPS (excluding blocked I/O)\n";
    }

    std::cout << "\n--- Per-opcode breakdown ---\n";
    std::cout
        << std::left  << std::setw(10) << "Opcode"
        << std::right << std::setw(10) << "Count"
        << std::setw(9) << "CPU %"
        << std::setw(14) << "Total"
        << std::setw(14) << "Avg"
        << std::setw(14) << "Min"
        << std::setw(14) << "Max"
        << std::setw(14) << "Blocked#"
        << std::setw(16) << "BlockedTot"
        << "\n";

    // Sort by total time descending - hottest opcodes first.
    std::vector<std::pair<int, OpcodeProfile>> rows(
        g_profiler.byOpcode.begin(), g_profiler.byOpcode.end());
    std::sort(rows.begin(), rows.end(), [](const auto& a, const auto& b) {
        return a.second.totalMs > b.second.totalMs;
    });

    for (const auto& [opcode, prof] : rows) {
        double avg = prof.count ? prof.totalMs / prof.count : 0.0;

        double cpuPercent =
            g_profiler.totalTimedMs > 0.0
                ? (prof.totalMs / g_profiler.totalTimedMs) * 100.0
                : 0.0;

        std::cout
            << std::left << std::setw(10) << opcodeName(opcode)
            << std::right << std::setw(10) << prof.count
            << std::setw(9) << std::fixed << std::setprecision(1)
            << cpuPercent << "%"
            << std::setw(14) << formatDuration(prof.totalMs)
            << std::setw(14) << formatDuration(avg)
            << std::setw(14) << formatDuration(prof.count ? prof.minMs : 0.0)
            << std::setw(14) << formatDuration(prof.maxMs)
            << std::setw(14) << prof.blockingCount
            << std::setw(16) << formatDuration(prof.blockingTotalMs)
            << "\n";
    }

    if (g_profiler.totalBlockingMs > 0.0) {
        std::cout << "\nNote: instructions taking longer than "
                   << BLOCKING_SAMPLE_THRESHOLD_MS
                   << " ms were assumed to be blocked on user input/IO "
                   << "(e.g. inputInt/inputString) and excluded from timing "
                   << "stats above; see Blocked# / BlockedTot(ms) columns.\n";
    }

    if (!rows.empty())
    {
        const auto& [opcode, prof] = rows.front();

        double cpuPercent =
            g_profiler.totalTimedMs > 0.0
                ? (prof.totalMs / g_profiler.totalTimedMs) * 100.0
                : 0.0;

        std::cout << "\nTop hotspot\n";
        std::cout << "-----------\n";
        std::cout << "Opcode: " << opcodeName(opcode) << "\n";
        std::cout << "CPU Time: "
                << std::fixed << std::setprecision(1)
                << cpuPercent << "%\n";
        std::cout << "Executions: "
                << prof.count << "\n";
        std::cout << "Total CPU time: "
          << formatDuration(prof.totalMs) << "\n";
    }

    std::cout << "=============================\n" << std::endl;
}

// ---------------------------------------------------------------------------

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
        } else if(command == "profiling") {
            g_profiler.enabled = !g_profiler.enabled;
            std::cout << "Profiling " << (g_profiler.enabled ? "enabled" : "disabled") << std::endl;
        } else if(command == "run") {
            if(resume) {
                bool result = askYesNo("This will restart execution. Are you sure?", false);
                if(!result) continue;
            }

            if(g_profiler.enabled && !breakpoints.empty()) {
                std::cout << "Profiling cannot run with breakpoints set. "
                             "Clear breakpoints ('breakpoint clear') or disable "
                             "profiling ('profiling') before running." << std::endl;
                continue;
            }

            std::cout << "Executing..." << std::endl;

            zeroExecData(&execData, progData);

            if(g_profiler.enabled) {
                g_profiler.reset();
                g_profiler.beginRun();

                while(true) {
                    auto opcodeBefore = progData->bytecode[execData.PC];

                    auto t0 = std::chrono::steady_clock::now();
                    step(progData, &execData);
                    auto t1 = std::chrono::steady_clock::now();

                    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
                    g_profiler.recordInstruction(static_cast<int>(opcodeBefore), ms);

                    if(execData.halt) break;
                }

                resume = !execData.halt;
                if(execData.halt) std::cout << "Execution finished" << std::endl;

                printProfilingReport();
            } else {
                begin_execution(
                    progData, &execData, breakpoints
                );

                resume = !execData.halt;

                if(execData.halt) std::cout << "Execution finished" << std::endl;
            }
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
                if(g_profiler.enabled) {
                    std::cout << "Note: profiling is enabled and cannot run while "
                                 "breakpoints are set - disable profiling or clear "
                                 "breakpoints before 'run'." << std::endl;
                }
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
                if(g_profiler.enabled) {
                    auto opcodeBefore = progData->bytecode[execData.PC];
                    auto t0 = std::chrono::steady_clock::now();
                    step(progData, &execData);
                    auto t1 = std::chrono::steady_clock::now();
                    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
                    g_profiler.recordInstruction(static_cast<int>(opcodeBefore), ms);
                    std::cout << "(" << ms << " ms)" << std::endl;
                } else {
                    step(
                        progData, &execData
                    );
                }
            }
        } else if(command == "continue") {
            if(!resume) {
                std::cout << "Execution has not started. Use 'run' first." << std::endl;
            } else {
                if(g_profiler.enabled && !breakpoints.empty()) {
                    std::cout << "Profiling cannot run with breakpoints set. "
                                 "Clear breakpoints or disable profiling first." << std::endl;
                    continue;
                }

                step(
                    progData, &execData
                );

                if(g_profiler.enabled) {
                    while(true) {
                        auto opcodeBefore = progData->bytecode[execData.PC];
                        auto t0 = std::chrono::steady_clock::now();
                        step(progData, &execData);
                        auto t1 = std::chrono::steady_clock::now();
                        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
                        g_profiler.recordInstruction(static_cast<int>(opcodeBefore), ms);
                        if(execData.halt) break;
                    }
                    printProfilingReport();
                } else {
                    begin_execution(
                        progData, &execData, breakpoints
                    );
                }

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
    std::cout << "profiling - Toggle instruction-level profiling" << std::endl;
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