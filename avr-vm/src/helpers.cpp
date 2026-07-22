#include "includes.h"
#include "program.h"
#include "vm.h"
#include "vmfuncmap.h"
#include "uart.h"
#include "vmnativefuncmap.h"

float getNumeric(const Variant* v) {
  if (v->type == TAG_FLOAT) return v->data.f;
  return (float)v->data.i;
}

int isFloatVariant(const Variant* a, const Variant* b) {
  return a->type == TAG_FLOAT || b->type == TAG_FLOAT;
}

inline uint8_t readByte(const uint8_t* data, int* pos) {
    return pgm_read_byte(&data[(*pos)++]);
}

int readInt(const uint8_t* data, int* pos) {
    uint32_t b0 = readByte(data, pos);
    uint32_t b1 = readByte(data, pos);
    uint32_t b2 = readByte(data, pos);
    uint32_t b3 = readByte(data, pos);
    return (int)(b0 | (b1 << 8) | (b2 << 16) | (b3 << 24));
}

float readfloat(const uint8_t* data, int* pos) {
    uint8_t b[8];
    for (int i = 0; i < 8; i++) {
        b[i] = readByte(data, pos);
    }

    uint64_t bits = 0;
    for (int i = 0; i < 8; i++) {
        bits |= ((uint64_t)b[i]) << (i * 8);
    }

    uint8_t sign = (bits >> 63) & 0x01;
    int16_t exp = ((bits >> 52) & 0x7FF) - 1023;
    uint64_t mant = bits & 0x000FFFFFFFFFFFFFULL;

    if (exp == -1023) return 0.0f;

    double doubleVal = (1.0 + ((double)mant / 4503599627370496.0));

    while (exp > 0) { doubleVal *= 2.0; exp--; }
    while (exp < 0) { doubleVal /= 2.0; exp++; }

    if (sign) doubleVal = -doubleVal;

    return (float)doubleVal;
}

uint32_t readU32FromBytecode(const uint8_t* bytecode, int pos) {
  return (uint32_t)bytecode[pos] | (uint32_t)bytecode[pos + 1] << 8 | (uint32_t)bytecode[pos + 2] << 16 | (uint32_t)bytecode[pos + 3] << 24;
}

int getOpCodeOffset(int opcode) {
  switch (opcode) {
    case 0x03:
      return 3;
    case 0x04:
    case 0x02:
    case 0xB0:
    case 0xB1:
    case 0xB2:
    case 0xB3:
    case 0xB4:
    case 0xB5:
    case 0x05:
    case 0x01:
      return 2;
    case 0xFF:
    case 0xA0:
    case 0xA1:
    case 0xA2:
    case 0xA3:
    case 0xA4:
    case 0xA5:
    case 0xAA:
    case 0xFE:
      return 1;
    case 0x06:
    case 0x07:
    case 0xC0:
    case 0xC1:
    case 0xC2:
    case 0xC3:
    case 0xC4:
    case 0xC5:
      return 5;
  }
  return 1;
}

uint8_t* getDDR(uint8_t pin) {
    if (pin <= 7) return &DDRD;
    if (pin <= 13) return &DDRB;
    return &DDRC;
}

uint8_t* getPORT(uint8_t pin) {
    if (pin <= 7) return &PORTD;
    if (pin <= 13) return &PORTB;
    return &PORTC;
}

uint8_t* getPIN(uint8_t pin) {
    if (pin <= 7) return &PIND;
    if (pin <= 13) return &PINB;
    return &PINC;
}

uint8_t getBit(uint8_t pin) {
    if (pin <= 7) return pin;
    if (pin <= 13) return pin - 8;
    return pin - 14;
}