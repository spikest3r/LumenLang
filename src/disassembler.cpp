#include "disassembler.h"

enum OperandType {
    NONE = 0,
    TYPE = 1,
    VALUE = 2
};

std::map<int, std::string> disassemblyMap = {
    {0x02, "POP"},
    {0x03, "PUSH"},
    {0x04, "EXEC"},
    {0x05, "JUMP"},
    
    {0xA0, "ADD"},
    {0xA1, "SUB"},
    {0xA2, "MUL"},
    {0xA3, "DIV"},
    {0xA4, "POW"},
    {0xA5, "MOD"},
    
    {0xB0, "JEQ"},
    {0xB1, "JGR"},
    {0xB2, "JLS"},
    {0xB3, "JGE"},
    {0xB4, "JLE"},
    {0xB5, "JNE"},

    {0xFF, "HLT"}
};

void disassemble(std::vector<int> bytecode) {
    int PC = 0;
    int size = bytecode.size();
    
    while(PC < size) {
        auto offset = getOpCodeOffset(bytecode[PC]);

        std::cout << "0x" << std::hex << std::right << std::setw(8) << std::setfill('0') << PC << ": ";

        std::stringstream hex_stream;
        for(int i = PC; i < PC + offset; i++) {
            hex_stream << std::hex << std::setw(2) << std::setfill('0')
                    << static_cast<int>(bytecode[i]) << " ";
        }

        std::cout << std::setfill(' ') << std::left << std::setw(15) << hex_stream.str() << " | ";

        std::cout << std::dec;

        auto op = disassemblyMap[bytecode[PC]];
        std::cout << op;

        for(int i = PC + 1; i < PC + offset; i++) {
            if(!op.empty() && op[0] == 'J') {
                std::cout << ", 0x" 
                        << std::hex << std::right << std::setw(8) << std::setfill('0')
                        << static_cast<int>(bytecode[i]);
                
                std::cout << std::dec << std::setfill(' ') << std::left; 
            } else {
                std::cout << ", " << std::dec << static_cast<int>(bytecode[i]);
            }
        }


        std::cout << std::endl;
        
        PC += offset;
    }

    std::cout << std::dec << std::endl;
}