#include "vmnativefuncmap.h"
#include "helpers.h"
#include "uart.h"

void fn_gpioInit(Variant stack[16], Variant variables[16], int* sp) {
    Variant* arg = &stack[*sp];
    (*sp)--;

    uint8_t pin = arg->data.i;

    uint8_t* ddr = getDDR(pin);
    uint8_t bit = getBit(pin);

    *ddr &= ~(1 << bit);
}

void fn_gpioSetDir(Variant stack[16], Variant variables[16], int* sp) {
    Variant* pin_value = &stack[*sp];
    (*sp)--;

    Variant* pin_num = &stack[*sp];
    (*sp)--;

    uint8_t pin = pin_num->data.i;
    uint8_t mode = pin_value->data.i;

    uint8_t* ddr = getDDR(pin);
    uint8_t bit = getBit(pin);

    if (mode == 1)
        *ddr |= (1 << bit);
    else
        *ddr &= ~(1 << bit);
}

void fn_gpioPut(Variant stack[16], Variant variables[16], int* sp) {
    Variant* pin_value = &stack[*sp];
    (*sp)--;

    Variant* pin_num = &stack[*sp];
    (*sp)--;

    uint8_t pin = pin_num->data.i;

    uint8_t* port = getPORT(pin);
    uint8_t bit = getBit(pin);

    if (pin_value->data.i)
        *port |= (1 << bit);
    else
        *port &= ~(1 << bit);
}

void fn_gpioGet(Variant stack[16], Variant variables[16], int* sp) {
    Variant* varRef = &stack[*sp];
    (*sp)--;

    Variant* pin_num = &stack[*sp];
    (*sp)--;

    uint8_t pin = pin_num->data.i;

    uint8_t* input = getPIN(pin);
    uint8_t bit = getBit(pin);

    int value = (*input & (1 << bit)) ? 1 : 0;

    variables[varRef->data.i].type = TAG_INT;
    variables[varRef->data.i].data.i = value;
}

void fn_gpioPullUp(Variant stack[16], Variant variables[16], int* sp) {
    Variant* pin_num = &stack[*sp];
    (*sp)--;

    uint8_t pin = pin_num->data.i;

    uint8_t* port = getPORT(pin);
    uint8_t bit = getBit(pin);

    *port |= (1 << bit);
}

void fn_gpioPullDown(Variant stack[16], Variant variables[16], int* sp) {
    Variant* pin_num = &stack[*sp];
    (*sp)--;

    send_uart("gpioPullDown is unsupported on this platform! Hanging...\n");
    while(1);
}

void fn_sleepMs(Variant stack[16], Variant variables[16], int* sp) { 
    Variant* arg = &stack[*sp]; 
    (*sp)--; 

    int32_t ms = (int32_t)getNumeric(arg); 

    for (int32_t i = 0; i < ms; i++) {
        _delay_ms(1);
    }
}

void fn_sleepUs(Variant stack[16], Variant variables[16], int* sp) { 
    Variant* arg = &stack[*sp]; 
    (*sp)--; 

    int32_t us = (int32_t)getNumeric(arg); 

    for (int32_t i = 0; i < us; i++) {
        _delay_us(1);
    }
}

// one wire

#include <stdint.h>


// =========================
// 1-Wire GPIO helpers
// =========================

static void oneWireLow(uint8_t pin) {
    uint8_t* ddr = getDDR(pin);
    uint8_t* port = getPORT(pin);
    uint8_t bit = getBit(pin);

    *port &= ~(1 << bit);   // output low
    *ddr |= (1 << bit);
}


static void oneWireRelease(uint8_t pin) {
    uint8_t* ddr = getDDR(pin);
    uint8_t bit = getBit(pin);

    *ddr &= ~(1 << bit);    // input, release bus
}


static uint8_t oneWireReadPin(uint8_t pin) {
    uint8_t* input = getPIN(pin);
    uint8_t bit = getBit(pin);

    return (*input & (1 << bit)) ? 1 : 0;
}


// =========================
// Reset / presence
// =========================

uint8_t oneWireReset(uint8_t pin) {
    oneWireLow(pin);
    _delay_us(480);

    oneWireRelease(pin);
    _delay_us(70);

    uint8_t line = oneWireReadPin(pin);

    _delay_us(410);

    if (line == 0)
        return 1;
    else
        return 0;
}


// =========================
// Bit operations
// =========================

static void oneWireWriteBit(uint8_t pin, uint8_t bit) {

    if (bit) {
        // write 1
        oneWireLow(pin);
        _delay_us(6);

        oneWireRelease(pin);
        _delay_us(64);
    }
    else {
        // write 0
        oneWireLow(pin);
        _delay_us(60);

        oneWireRelease(pin);
        _delay_us(10);
    }
}


static uint8_t oneWireReadBit(uint8_t pin) {

    uint8_t value;

    oneWireLow(pin);
    _delay_us(6);

    oneWireRelease(pin);
    _delay_us(9);

    value = oneWireReadPin(pin);

    _delay_us(55);

    return value;
}


// =========================
// Byte operations
// =========================

void oneWireWriteByte(uint8_t pin, uint8_t value) {

    for (uint8_t i = 0; i < 8; i++) {

        oneWireWriteBit(pin, value & 1);

        value >>= 1;
    }
}


uint8_t oneWireReadByte(uint8_t pin) {

    uint8_t value = 0;

    for (uint8_t i = 0; i < 8; i++) {

        if (oneWireReadBit(pin))
            value |= (1 << i);
    }

    return value;
}

void fn_oneWireReset(Variant stack[16], Variant variables[16], int* sp) {
    Variant* varRef = &stack[*sp];
    (*sp)--;

    Variant* pinArg = &stack[*sp];
    (*sp)--;

    uint8_t pin = pinArg->data.i;

    uint8_t present = oneWireReset(pin);

    variables[varRef->data.i].type = TAG_INT;
    variables[varRef->data.i].data.i = present;
}

void fn_oneWireWriteByte(Variant stack[16], Variant variables[16], int* sp) {
    Variant* valueArg = &stack[*sp];
    (*sp)--;

    Variant* pinArg = &stack[*sp];
    (*sp)--;

    uint8_t pin = pinArg->data.i;
    uint8_t value = valueArg->data.i;

    oneWireWriteByte(pin, value);
}

void fn_oneWireReadByte(Variant stack[16], Variant variables[16], int* sp) {
    Variant* varRef = &stack[*sp];
    (*sp)--;

    Variant* pinArg = &stack[*sp];
    (*sp)--;

    uint8_t pin = pinArg->data.i;

    uint8_t value = oneWireReadByte(pin);

    variables[varRef->data.i].type = TAG_INT;
    variables[varRef->data.i].data.i = value;
}

void fn_shiftLeft(Variant stack[16], Variant variables[16], int* sp) {
    Variant* varRef = &stack[*sp];
    (*sp)--;

    Variant* shift = &stack[*sp];
    (*sp)--;

    Variant* value = &stack[*sp];
    (*sp)--;

    int32_t result = value->data.i << shift->data.i;

    variables[varRef->data.i].type = TAG_INT;
    variables[varRef->data.i].data.i = result;
}

void fn_shiftRight(Variant stack[16], Variant variables[16], int* sp) {
    Variant* varRef = &stack[*sp];
    (*sp)--;

    Variant* shift = &stack[*sp];
    (*sp)--;

    Variant* value = &stack[*sp];
    (*sp)--;

    int32_t result = value->data.i >> shift->data.i;

    variables[varRef->data.i].type = TAG_INT;
    variables[varRef->data.i].data.i = result;
}

void fn_or(Variant stack[16], Variant variables[16], int* sp) {
    Variant* varRef = &stack[*sp];
    (*sp)--;

    Variant* b = &stack[*sp];
    (*sp)--;

    Variant* a = &stack[*sp];
    (*sp)--;

    int32_t result = a->data.i | b->data.i;

    variables[varRef->data.i].type = TAG_INT;
    variables[varRef->data.i].data.i = result;
}