#include "compiler_internal.h"

int finalizeCompile(CompileState& state, CompilerData* compilerData,
    bool debugInfo, const std::string& fileName, bool subscript
) {
    // if its subcript, skip HALT append
    if(!subscript) {
        // Appending HALT instruction to main bytecode
        compilerData->bytecode.push_back(0xFF); // HALT
    }

    // Resolve Main Program Jumps
    for (const auto& it : state.unresolvedJumps) {
        if (it.routineIndex == -1) {
            auto it2 = state.globalLabels.find(it.keyword);
            if (it2 != state.globalLabels.end()) {
                patchUint32(compilerData->bytecode, it.location, static_cast<uint32_t>(it2->second));
            }
            else {
                printError("Label '" + it.keyword + "' is not defined in global scope", it.line, state.ownFilename.back());
                return -1;
            }
        }
    }

    if(subscript) return 0;

    // Append Subroutine Bytecodes and Calculate Final Absolute Offsets
    std::unordered_map<int, int> routineOffsets;

    for (const auto& it : state.subroutineBytecode) {
        auto idx = it.first;
        auto& routineBc = it.second;
        routineOffsets[idx] = static_cast<int>(compilerData->bytecode.size()); // Final offset where this routine starts

        // Resolve unresolved jumps inside this routine using scoped routine labels
        for (auto& jump : state.unresolvedJumps) {
            if (jump.routineIndex == idx) {
                auto& rMap = state.routineLabels[idx];
                auto labelIt = rMap.find(jump.keyword);
                if (labelIt != rMap.end()) {
                    // relative offset within the routine — VM adds routineBase at runtime
                    uint32_t relativeTarget = static_cast<uint32_t>(labelIt->second);
                    patchUint32(state.subroutineBytecode[idx], jump.location, relativeTarget);
                }
                else {
                    printError("Label '" + jump.keyword + "' is not defined in routine scope", jump.line, state.ownFilename.back());
                    return -1;
                }
            }
        }
        // Merge routine bytecode into main global bytecode vector
        compilerData->bytecode.insert(compilerData->bytecode.end(), routineBc.begin(), routineBc.end());
    }

    // Patch Subroutine Calls (CALL32)
    for (const auto& it : state.unresolvedRoutineCalls) {
        auto keyword = it.keyword;
        auto location = it.location;
        auto line = it.line;

        auto it2 = state.subroutineIndexMap.find(keyword);

        if (it2 != state.subroutineIndexMap.end()) {
            int rIdx = it2->second;
            uint32_t absAddress = static_cast<uint32_t>(routineOffsets[rIdx]);

            if (it.routineIndex == -1) {
                patchUint32(compilerData->bytecode, location, absAddress);
            }
            else {
                // Adjust for target routine in subroutineBytecode chunk before merging
                int absoluteLocationInGlobalBytecode = routineOffsets[it.routineIndex] + location;
                patchUint32(compilerData->bytecode, absoluteLocationInGlobalBytecode, absAddress);
            }
        }
        else {
            printError("Subroutine '" + keyword + "' is not defined", line, state.ownFilename.back());
            return -1;
        }
    }

    for (const auto& it : compilerData->pendingRoutineCalls) {
        auto sigIt = compilerData->routineList.find(it.name);

        if (sigIt != compilerData->routineList.end()) {
            int rIdx = sigIt->second.index;
            uint32_t absAddress = static_cast<uint32_t>(routineOffsets[rIdx]);

            if (it.routineIndex == -1) {
                patchUint32(compilerData->bytecode, it.location, absAddress);
            }
            else {
                int absoluteLocationInGlobalBytecode = routineOffsets[it.routineIndex] + it.location;
                patchUint32(compilerData->bytecode, absoluteLocationInGlobalBytecode, absAddress);
            }
        }
        else {
            printError("Routine '" + it.name + "' is not defined", -1, state.ownFilename.back());
            return -1;
        }
    }

    if (debugInfo) {
        std::stringstream debugFile;

        // Write variable names and their indices
        debugFile << "variables" << std::endl;
        for (const auto& var : compilerData->variableMap) {
            debugFile << var.first << " " << var.second << std::endl;
        }
        // Write subroutine names, their bytecode offsets and bytecode length
        debugFile << "routines" << std::endl;
        for (const auto& sub : state.subroutineIndexMap) {
            debugFile << sub.first << std::endl;
            debugFile << routineOffsets[sub.second] << std::endl;
            debugFile << state.subroutineBytecode[sub.second].size() << std::endl;
        }
        // Write exec functions
        debugFile << "exec" << std::endl;
        for (const auto& func : funcList) {
            debugFile << func.first << " " << func.second.opcode << std::endl;
        }

        compilerData->debugData = debugFile.str();
    }

    compilerData->variableCount = static_cast<int>(compilerData->variableMap.size());

    return 0;
}
