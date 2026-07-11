#include <cstdint>
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: bin2h <input.bin> <output.h>\n";
        return -1;
    }

    std::ifstream in(argv[1], std::ios::binary);
    if (!in) {
        std::cerr << "Failed to open input file\n";
        return -1;
    }

    std::vector<uint8_t> bytes(
        (std::istreambuf_iterator<char>(in)),
        (std::istreambuf_iterator<char>())
    );
    in.close();

    std::ofstream out(argv[2]);
    if (!out) {
        std::cerr << "Failed to open output file\n";
        return -1;
    }

    out << "#pragma once\n";
    out << "#include <stdint.h>\n\n";
    out << "const uint32_t program[] = {\n    ";

    for (size_t i = 0; i < bytes.size(); i++) {
        out << "0x" << std::hex << std::setw(2) << std::setfill('0')
            << (int)bytes[i];
        if (i + 1 < bytes.size()) out << ", ";
        if ((i + 1) % 16 == 0) out << "\n    ";
    }

    out << "\n};\n\n";
    out << "const int programSize = " << std::dec << bytes.size() << ";\n";

    out.close();
    std::cout << "Written " << bytes.size() << " bytes to " << argv[2] << "\n";
    return 0;
}
