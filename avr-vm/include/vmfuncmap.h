#pragma once

#include "includes.h"

void fn_println(Variant stack[16], Variant variables[16], int* sp);
void fn_print(Variant stack[16], Variant variables[16], int* sp);
void fn_inputInt(Variant stack[16], Variant variables[16], int* sp);
void fn_inputStr(Variant stack[16], Variant variables[16], int* sp);
void fn_str2int(Variant stack[16], Variant variables[16], int* sp);
void fn_int2str(Variant stack[16], Variant variables[16], int* sp);
void fn_str2float(Variant stack[16], Variant variables[16], int* sp);
void fn_float2str(Variant stack[16], Variant variables[16], int* sp);