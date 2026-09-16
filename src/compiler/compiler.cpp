#include "compiler_internal.h"

int compileFromStream(std::istream& input,
    CompilerData* compilerData,
    bool verbose, bool debugInfo, std::string fileName
) {
    std::string line;

    std::vector<std::string> allLines;
    while (std::getline(input, line)) {
        allLines.push_back(line);
    }
    prescanRoutines(allLines, compilerData);

    CompileState state;

    for (const std::string& currentLine : allLines) {
        int result = compileLine(currentLine, state, compilerData, verbose, debugInfo);
        if (result != 0) return result;
    }

    return finalizeCompile(state, compilerData, debugInfo, fileName);
}

int compileFromFile(std::ifstream& file,
    CompilerData* compilerData,
    bool verbose, bool debugInfo, std::string fileName
) {
    if (!file.is_open()) {
        std::cerr << "File is not open" << std::endl;
        return -1;
    }
    return compileFromStream(file, compilerData, verbose, debugInfo, fileName);
}

int compileFromText(const std::string& text,
    CompilerData* compilerData,
    bool verbose, bool debugInfo, std::string fileName
) {
    std::istringstream stream(text);
    return compileFromStream(stream, compilerData, verbose, debugInfo, fileName);
}

int compile(std::string fileName,
    CompilerData* compilerData,
    bool verbose, bool debugInfo
) {
    std::ifstream file(fileName);
    if (!file.is_open()) {
        std::cerr << "Could not open file: " << fileName << std::endl;
        return -1;
    }
    int result = compileFromFile(file, compilerData, verbose, debugInfo, fileName);
    file.close();
    return result;
}
