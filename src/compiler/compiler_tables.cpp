#include "compiler_internal.h"

const std::unordered_map<std::string, ConditionOp> condOpMap = {
    {"==", EQUALS},
    {">",  GREATER},
    {"<",  LESSER},
    {">=", GREATER_OR_EQ},
    {"<=", LESSER_OR_EQ},
    {"!=", NOT_EQUALS}
};

const std::unordered_map<ConditionOp, uint8_t> condOpcodeMap = {
    {EQUALS,        0xC0},
    {GREATER,       0xC1},
    {LESSER,        0xC2},
    {GREATER_OR_EQ, 0xC3},
    {LESSER_OR_EQ,  0xC4},
    {NOT_EQUALS,    0xC5}
};

std::unordered_map<std::string, Function> funcList = {
    {"println",   {0x01,1,0}},
    {"print",     {0x02,1,0}},
    {"inputInt",  {0x03,1,1}},
    {"inputStr",  {0x04,0,1}},
    {"str2int",   {0x05,1,1}},
    {"int2str",   {0x06,1,1}},
    {"str2float", {0x07,1,1}},
    {"float2str", {0x08,1,1}},
    {"assertCapability", {0xA0,1,0}},
    {"openFile", {0xA1,1,1}},
    {"writeFile", {0xA2, 2,0}},
    {"readFile", {0xA3, 1,1}},
    {"closeFile", {0xA4, 1,0}},
    {"randomSeed", {0xA5, 1,0}},
    {"random", {0xA6, 0,1}},
    {"randomRange", {0xA7, 2,1}},
    {"httpRequest", {0xA8, 5, 1}},
    {"strlen", {0xA9, 1,1}},
    {"substr", {0xAA, 3,1}},
    {"strfind", {0xAB, 2,1}},
    {"strcase", {0xAC, 2,1}},
    {"trim", {0xAD, 1,1}}
};
