#include "programfile.h"

bool BinaryProgram::save(const std::string& path) {
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;

    // signature FE FB (v2)
    unsigned char sig[2] = {0xFE, 0xFB};
    out.write(reinterpret_cast<char*>(sig), 2);

    // bytecode
    int bcSize = (int)bytecode.size();
    out.write(reinterpret_cast<char*>(&bcSize), sizeof(int));
    out.write(reinterpret_cast<char*>(bytecode.data()), bcSize * sizeof(uint8_t));

    // string pool
    int spSize = (int)stringPool.size();
    out.write(reinterpret_cast<char*>(&spSize), sizeof(int));

    for (const auto& str : stringPool) {
        int len = (int)str.size();
        out.write(reinterpret_cast<char*>(&len), sizeof(int));
        out.write(str.data(), len);
    }
    
    // const pool
    int cpSize = (int)constPool.size();
    out.write(reinterpret_cast<char*>(&cpSize), sizeof(int));
    out.write(reinterpret_cast<char*>(constPool.data()), cpSize * sizeof(int));

    // variable index
    out.write(reinterpret_cast<char*>(&variableCount), sizeof(int));

    return true;
}

bool BinaryProgram::load(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;

    // signature
    unsigned char sig[2];
    in.read(reinterpret_cast<char*>(sig), 2);

    // FE FA (v1)
    // FE FB (v2)

    if (sig[0] != 0xFE || sig[1] != 0xFB) {
        std::cerr << "Invalid signature\n";
        if(sig[1] == 0xFA) {
            std::cout << "v1 Precompiled Lumen binaries are not compatible with v2 Lumen runtime" << std::endl;
        }
        return false;
    }

    // bytecode
    int bcSize = 0;
    in.read(reinterpret_cast<char*>(&bcSize), sizeof(int));

    bytecode.resize(bcSize);
    in.read(reinterpret_cast<char*>(bytecode.data()), bcSize * sizeof(uint8_t));

    // string pool
    int spSize = 0;
    in.read(reinterpret_cast<char*>(&spSize), sizeof(int));

    stringPool.clear();
    stringPool.reserve(spSize);

    for (int i = 0; i < spSize; i++) {
        int len = 0;
        in.read(reinterpret_cast<char*>(&len), sizeof(int));

        std::string str(len, '\0');
        in.read(&str[0], len);

        stringPool.push_back(str);
    }

    // constPool
    int cpSize = 0;
    in.read(reinterpret_cast<char*>(&cpSize), sizeof(int));

    constPool.resize(cpSize);
    in.read(reinterpret_cast<char*>(constPool.data()), cpSize * sizeof(int));

    // variable count
    in.read(reinterpret_cast<char*>(&variableCount), sizeof(int));

    return true;
}

void constructProgData(VMProgramData* progData, BinaryProgram* inProg) {
    progData->bytecode = inProg->bytecode;
    progData->stringPool = inProg->stringPool;
    progData->constPool = inProg->constPool;
    progData->variableCount = inProg->variableCount;
}