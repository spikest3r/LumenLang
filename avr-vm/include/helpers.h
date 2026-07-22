#pragma once

#include "includes.h"

float getNumeric(const Variant* v);
int isFloatVariant(const Variant* a, const Variant* b);
uint8_t readByte(const uint8_t* data, int* pos);
int readInt(const uint8_t* data, int* pos);
float readfloat(const uint8_t* data, int* pos);
uint32_t readU32FromBytecode(const uint8_t* bytecode, int pos);
int getOpCodeOffset(int opcode);
uint8_t* getDDR(uint8_t pin);
uint8_t* getPORT(uint8_t pin);
uint8_t* getPIN(uint8_t pin);
uint8_t getBit(uint8_t pin);