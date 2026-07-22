#pragma once

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#define MAX_STRINGS 16
#define MAX_STRING_LEN 64
#define MAX_CONST_POOL 32

typedef enum {
  TAG_STRING = 1,
  TAG_INT = 2,
  TAG_FLOAT = 3
} TypeTag;

typedef struct {
  TypeTag type;
  union {
    int32_t i;
    float f;
    char* str;
  } data;
} Variant;

typedef struct {
  int returnPC;
  int routineBase;
} CallFrame;