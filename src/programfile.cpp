#include "programfile.h"

bool BinaryProgram::save(const std::string& path) {
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;

    // signature FE FA
    unsigned char sig[2] = {0xFE, 0xFA};
    out.write(reinterpret_cast<char*>(sig), 2);

    // bytecode
    int bcSize = (int)bytecode.size();
    out.write(reinterpret_cast<char*>(&bcSize), sizeof(int));
    out.write(reinterpret_cast<char*>(bytecode.data()), bcSize * sizeof(int));

    // string pool
    int spSize = (int)stringPool.size();
    out.write(reinterpret_cast<char*>(&spSize), sizeof(int));

    for (const auto& str : stringPool) {
        int len = (int)str.size();
        out.write(reinterpret_cast<char*>(&len), sizeof(int));
        out.write(str.data(), len);
    }

    // variable index
    out.write(reinterpret_cast<char*>(&variableIndex), sizeof(int));

    return true;
}

bool BinaryProgram::load(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;

    // signature
    unsigned char sig[2];
    in.read(reinterpret_cast<char*>(sig), 2);
    if (sig[0] != 0xFE || sig[1] != 0xFA) {
        std::cerr << "Invalid signature\n";
        return false;
    }

    // bytecode
    int bcSize = 0;
    in.read(reinterpret_cast<char*>(&bcSize), sizeof(int));

    bytecode.resize(bcSize);
    in.read(reinterpret_cast<char*>(bytecode.data()), bcSize * sizeof(int));

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

    // variable index
    in.read(reinterpret_cast<char*>(&variableIndex), sizeof(int));

    return true;
}