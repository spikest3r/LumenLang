#pragma once

#include "includes.h"

void fn_gpioInit(Variant stack[16], Variant variables[16], int* sp);
void fn_gpioSetDir(Variant stack[16], Variant variables[16], int* sp);
void fn_gpioPut(Variant stack[16], Variant variables[16], int* sp);
void fn_gpioGet(Variant stack[16], Variant variables[16], int* sp);
void fn_gpioPullUp(Variant stack[16], Variant variables[16], int* sp);
void fn_gpioPullDown(Variant stack[16], Variant variables[16], int* sp);
void fn_sleepMs(Variant stack[16], Variant variables[16], int* sp);
void fn_sleepUs(Variant stack[16], Variant variables[16], int* sp);


void fn_oneWireReset(
    Variant stack[16],
    Variant variables[16],
    int* sp
);

void fn_oneWireWriteByte(
    Variant stack[16],
    Variant variables[16],
    int* sp
);

void fn_oneWireReadByte(
    Variant stack[16],
    Variant variables[16],
    int* sp
);

void fn_shiftLeft(
    Variant stack[16],
    Variant variables[16],
    int* sp
);

void fn_shiftRight(
    Variant stack[16],
    Variant variables[16],
    int* sp
);

void fn_or(
    Variant stack[16],
    Variant variables[16],
    int* sp
);