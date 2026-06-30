#pragma once
#include "includes.h"

typedef enum {
    TAG_INT = 2,
    TAG_FLOAT = 3,
    TAG_STRING = 1
} TypeTag;

typedef struct {
    TypeTag type;
    std::variant<int64_t, double, std::string> data;
} Variant;

typedef enum {
    NONE,
    ASSIGN,
    FUNC_CALL,
    PUSH_STACK,
    LABEL,
    JUMP,
    IF, ELSE
} Operation;

typedef enum {
    COP_NONE,
    EQUALS,
    GREATER,
    LESSER,
    GREATER_OR_EQ,
    LESSER_OR_EQ,
    NOT_EQUALS
} ConditionOp;