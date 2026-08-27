#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/timer.h"
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#define UART_ID uart1
#define BAUD_RATE 115200
#define UART_TX_PIN 4
#define UART_RX_PIN 5

#include "program.h"

#define MAX_STRINGS 16
#define MAX_STRING_LEN 64

#define MAX_CONST_POOL 256 // practical uint8_t limit

static uint8_t* g_bytecode = NULL;
static int g_bytecodeSize = 0;

static char* g_stringPool[MAX_STRINGS];
static int g_stringPoolSize = 0;

static double g_constPool[MAX_CONST_POOL];
static int g_constPoolSize = 0;

static int g_variableCount = 0;

static int halt = 0;

typedef enum {
    TAG_STRING = 1,
    TAG_INT = 2,
    TAG_FLOAT = 3
} TypeTag;

typedef struct {
    TypeTag type;
    union {
        int32_t i;
        double f;
        char* str;
    } data;
} Variant;

static double getNumeric(const Variant* v) {
    if(v->type == TAG_FLOAT) return v->data.f;
    return (double)v->data.i;
}

static int isFloatVariant(const Variant* a, const Variant* b) {
    return a->type == TAG_FLOAT || b->type == TAG_FLOAT;
}

void send_uart(const char* message) {
    uart_puts(UART_ID, message);
    printf("%s", message);
}

static void uart_put_variant(const Variant* v) {
    char buf[32];
    if(v->type == TAG_STRING) {
        send_uart(v->data.str);
        return;
    } else if(v->type == TAG_FLOAT) {
        snprintf(buf, sizeof(buf), "%f", v->data.f);
    } else {
        snprintf(buf, sizeof(buf), "%ld", (long)v->data.i);
    }
    send_uart(buf);
}

void uart_readline(char* buffer, int maxLen) {
    int i = 0;

    while(i < maxLen - 1) {
        int c = getchar();

        if(c == '\r' || c == '\n')
            break;

        buffer[i++] = c;
        putchar(c);
    }

    buffer[i] = '\0';
    printf("\n");
}

static int readInt(const uint8_t* data, int* pos) {
    int val = (int)(
        (uint32_t)data[*pos + 0]       |
        (uint32_t)data[*pos + 1] << 8  |
        (uint32_t)data[*pos + 2] << 16 |
        (uint32_t)data[*pos + 3] << 24
    );
    *pos += 4;
    return val;
}

static uint8_t readByte(const uint8_t* data, int* pos) {
    return data[(*pos)++];
}

static double readDouble(const uint8_t* data, int* pos) {
    double val;
    memcpy(&val, &data[*pos], sizeof(double));
    *pos += sizeof(double);
    return val;
}

int loadFromFlash(const uint8_t* data, int dataSize) {
    int pos = 0;

    uint8_t sig0 = data[pos++];
    uint8_t sig1 = data[pos++];

    // FE FA (v1) - not supported
    // FE FB (v2) - constPool of int32
    // FE FC (v3) - constPool of double
    // FE FD (v4) - 32-bit jump/call/comparison addressing
    int isV2 = (sig0 == 0xFE && sig1 == 0xFB);
    int isV3 = (sig0 == 0xFE && sig1 == 0xFC);
    int isV4 = (sig0 == 0xFE && sig1 == 0xFD);
    int isV5 = (sig0 == 0xFE && sig1 == 0xFE);

    if (!isV2 && !isV3 && !isV4 && !isV5) {
        send_uart("Invalid signature\n");
        if (sig0 == 0xFE && sig1 == 0xFA) {
            send_uart("v1 Precompiled Lumen binaries are not compatible with v2+ Lumen runtime\n");
        }
        return -1;
    }

    // bytecode
    int bcSize = readInt(data, &pos);
    if (g_bytecode) free(g_bytecode);
    g_bytecode = (uint8_t*)malloc(bcSize * sizeof(uint8_t));
    if (!g_bytecode) { send_uart("malloc failed\n"); return -1; }
    g_bytecodeSize = bcSize;
    for (int i = 0; i < bcSize; i++) {
        g_bytecode[i] = readByte(data, &pos);
    }

    // string pool
    int spSize = readInt(data, &pos);
    for (int i = 0; i < g_stringPoolSize; i++) {
        if (g_stringPool[i]) { free(g_stringPool[i]); g_stringPool[i] = NULL; }
    }
    g_stringPoolSize = spSize;
    for (int i = 0; i < spSize; i++) {
        int len = readInt(data, &pos);
        g_stringPool[i] = (char*)malloc(len + 1);
        if (!g_stringPool[i]) return -1;
        for (int j = 0; j < len; j++) {
            g_stringPool[i][j] = (char)readByte(data, &pos);
        }
        g_stringPool[i][len] = '\0';
    }

    // const pool
    int cpSize = readInt(data, &pos);
    g_constPoolSize = cpSize;
    if (!isV2) {
        for (int i = 0; i < cpSize; i++) {
            g_constPool[i] = readDouble(data, &pos);
        }
    } else {
        // v2: const pool entries are int32
        for (int i = 0; i < cpSize; i++) {
            g_constPool[i] = (double)readInt(data, &pos);
        }
    }

    // variable count
    g_variableCount = readInt(data, &pos);

    return 0;
}

static uint32_t readU32FromBytecode(const uint8_t* bytecode, int pos) {
    return (uint32_t)bytecode[pos]           |
           (uint32_t)bytecode[pos + 1] << 8  |
           (uint32_t)bytecode[pos + 2] << 16 |
           (uint32_t)bytecode[pos + 3] << 24;
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
    case 0xA8:
    case 0xA9:
    case 0xAB:
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
    case 0xDE:
    case 0xA6:
    case 0xA7:
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

typedef void (*NativeFn)(Variant stack[16], Variant variables[16], int* sp);

void fn_println(Variant stack[16], Variant variables[16], int* sp) {
    Variant* arg = &stack[*sp];
    (*sp)--;
    uart_put_variant(arg);
    send_uart("\n");
}

void fn_print(Variant stack[16], Variant variables[16], int* sp) {
    Variant* arg = &stack[*sp];
    (*sp)--;
    uart_put_variant(arg);
}

void fn_inputInt(Variant stack[16], Variant variables[16], int* sp) {
    Variant* varRef = &stack[*sp];
    (*sp)--;

    char buffer[32];
    uart_readline(buffer, sizeof(buffer));

    char* end;
    double value = strtod(buffer, &end);

    if (*end != '\0') {
        send_uart("Invalid value!\n");
        value = 0;
    }

    variables[varRef->data.i].type = TAG_INT;
    variables[varRef->data.i].data.i = (int32_t)value;
}

char buffer_inputStr[64];

void fn_inputStr(Variant stack[16], Variant variables[16], int* sp) {
    Variant* varRef = &stack[*sp];
    (*sp)--;

    memset(buffer_inputStr, 0, sizeof(buffer_inputStr));
    uart_readline(buffer_inputStr, sizeof(buffer_inputStr));

    variables[varRef->data.i].type = TAG_STRING;
    variables[varRef->data.i].data.str = buffer_inputStr;
}

void fn_str2int(Variant stack[16], Variant variables[16], int* sp) {
    Variant* varRef = &stack[*sp];
    (*sp)--;

    Variant* value = &stack[*sp];
    (*sp)--;

    int isStr = value->type == TAG_STRING;

    char* end;
    errno = 0;

    long val = isStr ? strtol(value->data.str, &end, 10) : 0;

    if (isStr && end == value->data.str) {
        val = 0;
    } else if (errno == ERANGE) {
        val = 0;
    }

    variables[varRef->data.i].type = TAG_INT;
    variables[varRef->data.i].data.i = (int32_t)val;
}

char buffer_int2str[12];

void fn_int2str(Variant stack[16], Variant variables[16], int* sp) {
    Variant* varRef = &stack[*sp];
    (*sp)--;

    Variant* value = &stack[*sp];
    (*sp)--;

    int32_t integer = 0;

    if(value->type == TAG_INT) integer = value->data.i;

    memset(buffer_int2str, 0, sizeof(buffer_int2str));
    snprintf(buffer_int2str, sizeof(buffer_int2str), "%ld", (long)integer);

    variables[varRef->data.i].type = TAG_STRING;
    variables[varRef->data.i].data.str = buffer_int2str;
}

void fn_str2float(Variant stack[16], Variant variables[16], int* sp) {
    Variant* varRef = &stack[*sp];
    (*sp)--;

    Variant* value = &stack[*sp];
    (*sp)--;

    int isStr = value->type == TAG_STRING;

    char* end;
    errno = 0;

    double val = isStr ? strtod(value->data.str, &end) : 0.0;

    if (isStr && end == value->data.str) {
        val = 0.0;
    } else if (errno == ERANGE) {
        val = 0.0;
    }

    variables[varRef->data.i].type = TAG_FLOAT;
    variables[varRef->data.i].data.f = val;
}

char buffer_float2str[32];

void fn_float2str(Variant stack[16], Variant variables[16], int* sp) {
    Variant* varRef = &stack[*sp];
    (*sp)--;

    Variant* value = &stack[*sp];
    (*sp)--;

    double num = 0.0;
    if(value->type == TAG_FLOAT) num = value->data.f;
    else if(value->type == TAG_INT) num = (double)value->data.i;

    memset(buffer_float2str, 0, sizeof(buffer_float2str));
    snprintf(buffer_float2str, sizeof(buffer_float2str), "%f", num);

    variables[varRef->data.i].type = TAG_STRING;
    variables[varRef->data.i].data.str = buffer_float2str;
}

void fn_gpioInit(Variant stack[16], Variant variables[16], int* sp) {
    Variant* arg = &stack[*sp];
    (*sp)--;
    gpio_init(arg->data.i);
}

void fn_gpioSetDir(Variant stack[16], Variant variables[16], int* sp) {
    Variant* pin_value = &stack[*sp]; // Popped second argument
    (*sp)--;
    Variant* pin_num = &stack[*sp];   // Popped first argument
    (*sp)--;

    gpio_set_dir(pin_num->data.i, pin_value->data.i == 1 ? GPIO_OUT : GPIO_IN);
}

void fn_gpioPut(Variant stack[16], Variant variables[16], int* sp) {
    Variant* pin_value = &stack[*sp]; // Popped second argument
    (*sp)--;
    Variant* pin_num = &stack[*sp];   // Popped first argument
    (*sp)--;
    
    gpio_put(pin_num->data.i, pin_value->data.i == 0 ? 0 : 1);
}

void fn_sleepMs(Variant stack[16], Variant variables[16], int* sp) {
    Variant* arg1 = &stack[*sp];
    (*sp)--;

    sleep_ms(arg1->data.i);
}

void fn_gpioGet(Variant stack[16], Variant variables[16], int* sp) {
    Variant* varRef = &stack[*sp];   // Popped second argument
    (*sp)--;

    Variant* pin_num = &stack[*sp]; // Popped first argument
    (*sp)--;

    int value = gpio_get(pin_num->data.i);
    
    variables[varRef->data.i].type = TAG_INT;
    variables[varRef->data.i].data.i = value;
}

void fn_gpioPullUp(Variant stack[16], Variant variables[16], int* sp) {
    Variant* pin_num = &stack[*sp];   // Popped first argument
    (*sp)--;

    gpio_pull_up(pin_num->data.i);
}

void fn_gpioPullDown(Variant stack[16], Variant variables[16], int* sp) {
    Variant* pin_num = &stack[*sp];   // Popped first argument
    (*sp)--;

    gpio_pull_down(pin_num->data.i);
}

void fn_assertCapability(Variant stack[16], Variant variables[16], int* sp) {
    // As of now, only "random" is implemented as a capability on Pico; FS and HTTP
    // are not available on this platform, so any other capability halts.
    // last value on stack is capability name

    Variant* capability = &stack[*sp];
    if(capability->type != TAG_STRING) {
        send_uart("assertCapability failed: invalid type\n");
        halt = 1;
        return;
    }
    (*sp)--;

    if(strcmp(capability->data.str, "random") == 0) {
        // capability present, proceed with execution
        return;
    }

    char buffer[256];
    snprintf(buffer, sizeof(buffer), "assertCapability failed: capability \'%s\' is not supported on this platform\n", capability->data.str);
    send_uart(buffer);
    halt = 1;
}

void fn_unsupportedCapability(Variant stack[16], Variant variables[16], int* sp) {
    send_uart("Error: this function is not supported on the Pico platform\n");
    halt = 1;
}

void fn_randomSeed(Variant stack[16], Variant variables[16], int* sp) {
    Variant* seed = &stack[*sp];
    (*sp)--;

    srand((unsigned int)seed->data.i);
}

void fn_random(Variant stack[16], Variant variables[16], int* sp) {
    Variant* varRef = &stack[*sp];
    (*sp)--;

    double val = (double)rand() / ((double)RAND_MAX + 1.0);

    variables[varRef->data.i].type = TAG_FLOAT;
    variables[varRef->data.i].data.f = val;
}

void fn_randomRange(Variant stack[16], Variant variables[16], int* sp) {
    Variant* varRef = &stack[*sp];
    (*sp)--;
    Variant* maxArg = &stack[*sp];
    (*sp)--;
    Variant* minArg = &stack[*sp];
    (*sp)--;

    int32_t min = minArg->data.i;
    int32_t max = maxArg->data.i;
    int32_t val = min + (int32_t)(rand() % (max - min + 1)); // inclusive on both ends

    variables[varRef->data.i].type = TAG_INT;
    variables[varRef->data.i].data.i = val;
}

void fn_strlen(Variant stack[16], Variant variables[16], int* sp) {
    Variant* varRef = &stack[*sp];
    (*sp)--;
    Variant* value = &stack[*sp];
    (*sp)--;

    int32_t len = (value->type == TAG_STRING) ? (int32_t)strlen(value->data.str) : 0;

    variables[varRef->data.i].type = TAG_INT;
    variables[varRef->data.i].data.i = len;
}

char buffer_substr[MAX_STRING_LEN];

void fn_substr(Variant stack[16], Variant variables[16], int* sp) {
    // substr(s, start, len, &out)
    Variant* varRef = &stack[*sp];
    (*sp)--;
    Variant* lenArg = &stack[*sp];
    (*sp)--;
    Variant* startArg = &stack[*sp];
    (*sp)--;
    Variant* value = &stack[*sp];
    (*sp)--;

    memset(buffer_substr, 0, sizeof(buffer_substr));

    if(value->type == TAG_STRING) {
        int32_t strLen = (int32_t)strlen(value->data.str);
        int32_t start = startArg->data.i;
        int32_t len = lenArg->data.i;

        if(start >= 0 && start < strLen) {
            int32_t maxLen = strLen - start;
            if(len > maxLen) len = maxLen;
            if(len > (int32_t)sizeof(buffer_substr) - 1) len = sizeof(buffer_substr) - 1;
            if(len > 0) memcpy(buffer_substr, value->data.str + start, len);
        }
    }

    variables[varRef->data.i].type = TAG_STRING;
    variables[varRef->data.i].data.str = buffer_substr;
}

void fn_strfind(Variant stack[16], Variant variables[16], int* sp) {
    // strfind(s, needle, &index)
    Variant* varRef = &stack[*sp];
    (*sp)--;
    Variant* needleVal = &stack[*sp];
    (*sp)--;
    Variant* strVal = &stack[*sp];
    (*sp)--;

    int32_t result = -1;
    if(strVal->type == TAG_STRING && needleVal->type == TAG_STRING) {
        char* found = strstr(strVal->data.str, needleVal->data.str);
        if(found) result = (int32_t)(found - strVal->data.str);
    }

    variables[varRef->data.i].type = TAG_INT;
    variables[varRef->data.i].data.i = result;
}

char buffer_toCase[MAX_STRING_LEN];

void fn_toCase(Variant stack[16], Variant variables[16], int* sp) {
    // toUpper(s, &out) / toLower(s, &out) via a flag arg (0=lower, 1=upper)
    Variant* varRef = &stack[*sp];
    (*sp)--;
    Variant* upperFlag = &stack[*sp];
    (*sp)--;
    Variant* value = &stack[*sp];
    (*sp)--;

    memset(buffer_toCase, 0, sizeof(buffer_toCase));

    if(value->type == TAG_STRING) {
        size_t len = strlen(value->data.str);
        if(len > sizeof(buffer_toCase) - 1) len = sizeof(buffer_toCase) - 1;
        for(size_t i = 0; i < len; i++) {
            unsigned char c = (unsigned char)value->data.str[i];
            buffer_toCase[i] = upperFlag->data.i ? (char)toupper(c) : (char)tolower(c);
        }
    }

    variables[varRef->data.i].type = TAG_STRING;
    variables[varRef->data.i].data.str = buffer_toCase;
}

char buffer_trim[MAX_STRING_LEN];

void fn_trim(Variant stack[16], Variant variables[16], int* sp) {
    // trim(s, &out)
    Variant* varRef = &stack[*sp];
    (*sp)--;
    Variant* value = &stack[*sp];
    (*sp)--;

    memset(buffer_trim, 0, sizeof(buffer_trim));

    if(value->type == TAG_STRING) {
        const char* ws = " \t\n\r\f\v";
        const char* str = value->data.str;
        size_t len = strlen(str);
        size_t start = strspn(str, ws);
        size_t end = len;
        while(end > start && strchr(ws, str[end - 1])) end--;

        size_t copyLen = (start < end) ? (end - start) : 0;
        if(copyLen > sizeof(buffer_trim) - 1) copyLen = sizeof(buffer_trim) - 1;
        if(copyLen > 0) memcpy(buffer_trim, str + start, copyLen);
    }

    variables[varRef->data.i].type = TAG_STRING;
    variables[varRef->data.i].data.str = buffer_trim;
}


NativeFn funcTable[] = {
    NULL,           // 0x00 unused
    fn_println,     // 0x01
    fn_print,       // 0x02
    fn_inputInt,    // 0x03
    fn_inputStr,    // 0x04
    fn_str2int,     // 0x05
    fn_int2str,     // 0x06
    fn_str2float,   // 0x07
    fn_float2str,   // 0x08
};

#define BASE_FUNC_COUNT (sizeof(funcTable) / sizeof(funcTable[0]))

NativeFn customFuncTable[] = {
    fn_gpioInit,      // 0xD0
    fn_gpioSetDir,    // 0xD1
    fn_gpioPut,       // 0xD2
    fn_sleepMs,       // 0xD3
    fn_gpioGet,       // 0xD4
    fn_gpioPullUp,    // 0xD5
    fn_gpioPullDown,  // 0xD6
};

#define CUSTOM_FUNC_COUNT (sizeof(customFuncTable) / sizeof(customFuncTable[0]))

NativeFn capabilityFuncTable[] = {
    fn_assertCapability,        // 0xA0
    fn_unsupportedCapability,   // 0xA1 openFile (no filesystem on Pico)
    fn_unsupportedCapability,   // 0xA2 writeFile
    fn_unsupportedCapability,   // 0xA3 readFile
    fn_unsupportedCapability,   // 0xA4 closeFile
    fn_randomSeed,              // 0xA5
    fn_random,                  // 0xA6
    fn_randomRange,             // 0xA7
    fn_unsupportedCapability,   // 0xA8 httpRequest (no network stack on Pico)
    fn_strlen,                  // 0xA9
    fn_substr,                  // 0xAA
    fn_strfind,                 // 0xAB
    fn_toCase,                  // 0xAC
    fn_trim,                    // 0xAD
};

#define CAPABILITY_FUNC_COUNT (sizeof(capabilityFuncTable) / sizeof(capabilityFuncTable[0]))

char joinBuffer[MAX_STRING_LEN];

int execute(
    const uint8_t* bytecode,
    const int bytecodeSize,
    const char* stringPool[],
    const int stringPoolSize
) {
    Variant variables[16];
    Variant stack[16];
    int pcStack[8];
    int routineBaseStack[8];
    int stackPointer = -1;
    int pcStackPointer = -1;
    int PC = 0;
    int routineBase = 0;

    while(1) {
        int opcode = bytecode[PC];
        int offset = getOpCodeOffset(opcode);
        switch(opcode) {
            case 0x01: {
                int addr = bytecode[PC + 1];
                pcStackPointer++;
                pcStack[pcStackPointer] = PC + offset;
                routineBaseStack[pcStackPointer] = routineBase;
                routineBase = addr;
                PC = addr;
                continue;
            }
            case 0xFE: {
                if(pcStackPointer < 0) {
                    send_uart("Return stack underflow!\n");
                    return -1;
                }
                PC = pcStack[pcStackPointer];
                routineBase = routineBaseStack[pcStackPointer];
                pcStackPointer--;
                continue;
            }
            case 0x02: {
                int varIndex = bytecode[PC + 1];
                if(stackPointer < 0) return -1;
                variables[varIndex].type = stack[stackPointer].type;
                switch(stack[stackPointer].type) {
                    case TAG_INT:
                        variables[varIndex].data.i = stack[stackPointer].data.i;
                        break;
                    case TAG_FLOAT:
                        variables[varIndex].data.f = stack[stackPointer].data.f;
                        break;
                    case TAG_STRING:
                        variables[varIndex].data.str = stack[stackPointer].data.str;
                        break;
                }
                stackPointer--;
                break;
            }
            case 0x03: {
                int dataType = bytecode[PC + 1];
                int value = bytecode[PC + 2];
                stackPointer++;
                Variant* v = &stack[stackPointer];
                switch(dataType) {
                    case 0x01:
                        v->type = TAG_STRING;
                        v->data.str = (char*)stringPool[value];
                        break;
                    case 0x02:
                        v->type = TAG_INT;
                        v->data.i = (int32_t)g_constPool[value];
                        break;
                    case 0x03: {
                        Variant* var = &variables[value];
                        v->type = var->type;
                        if(var->type == TAG_INT)
                            v->data.i = var->data.i;
                        else if(var->type == TAG_FLOAT)
                            v->data.f = var->data.f;
                        else
                            v->data.str = var->data.str;
                        break;
                    }
                    case 0x04: {
                        v->type = TAG_INT;
                        v->data.i = (int32_t)value;
                        break;
                    }
                    case 0x05: {
                        v->type = TAG_FLOAT;
                        v->data.f = g_constPool[value];
                        break;
                    }
                }
                break;
            }
            case 0x04: {
                int addr = bytecode[PC + 1];

                if(addr >= 0xD0 && addr <= 0xFF) {
                    int customIndex = addr - 0xD0;
                    if(customIndex < 0 || (unsigned)customIndex >= CUSTOM_FUNC_COUNT) {
                        send_uart("Invalid native call\n");
                        return -1;
                    }
                    if(customFuncTable[customIndex])
                        customFuncTable[customIndex](stack, variables, &stackPointer);
                } else if(addr >= 0xA0) {
                    // capabilities
                    int customIndex = addr - 0xA0;
                    // TODO: As capabilities for Pico platform grow, implement proper capability availability handler
                    if(customIndex < 0 || (unsigned)customIndex >= CAPABILITY_FUNC_COUNT) {
                        send_uart("Invalid native call\n");
                        return -1;
                    }
                    if(capabilityFuncTable[customIndex])
                        capabilityFuncTable[customIndex](stack, variables, &stackPointer);
                } else {
                    if(addr < 0 || (unsigned)addr >= BASE_FUNC_COUNT) {
                        send_uart("Invalid native call\n");
                        return -1;
                    }
                    if(funcTable[addr])
                        funcTable[addr](stack, variables, &stackPointer);
                }

                break;
            }
            case 0x05: {
                PC = routineBase + bytecode[PC + 1];
                continue;
            }
            case 0x06: { // JUMP32
                uint32_t addr = readU32FromBytecode(bytecode, PC + 1);
                PC = routineBase + (int)addr;
                continue;
            }
            case 0x07: { // CALL32
                uint32_t addr = readU32FromBytecode(bytecode, PC + 1);
                pcStackPointer++;
                pcStack[pcStackPointer] = PC + offset;
                routineBaseStack[pcStackPointer] = routineBase;
                routineBase = (int)addr;
                PC = (int)addr;
                continue;
            }
            case 0xA0: { // ADD
                if(stackPointer < 1) {
                    stackPointer++;
                    stack[stackPointer].type = TAG_INT;
                    stack[stackPointer].data.i = 0;
                    break;
                }
                Variant b = stack[stackPointer]; stackPointer--;
                Variant a = stack[stackPointer]; stackPointer--;
                stackPointer++;
                Variant* r = &stack[stackPointer];
                if(isFloatVariant(&a, &b)) {
                    r->type = TAG_FLOAT;
                    r->data.f = getNumeric(&a) + getNumeric(&b);
                } else {
                    r->type = TAG_INT;
                    r->data.i = a.data.i + b.data.i;
                }
                break;
            }
            case 0xA1: { // SUB
                if(stackPointer < 1) {
                    stackPointer++;
                    stack[stackPointer].type = TAG_INT;
                    stack[stackPointer].data.i = 0;
                    break;
                }
                Variant b = stack[stackPointer]; stackPointer--;
                Variant a = stack[stackPointer]; stackPointer--;
                stackPointer++;
                Variant* r = &stack[stackPointer];
                if(isFloatVariant(&a, &b)) {
                    r->type = TAG_FLOAT;
                    r->data.f = getNumeric(&a) - getNumeric(&b);
                } else {
                    r->type = TAG_INT;
                    r->data.i = a.data.i - b.data.i;
                }
                break;
            }
            case 0xA2: { // MUL
                if(stackPointer < 1) {
                    stackPointer++;
                    stack[stackPointer].type = TAG_INT;
                    stack[stackPointer].data.i = 0;
                    break;
                }
                Variant b = stack[stackPointer]; stackPointer--;
                Variant a = stack[stackPointer]; stackPointer--;
                stackPointer++;
                Variant* r = &stack[stackPointer];
                if(isFloatVariant(&a, &b)) {
                    r->type = TAG_FLOAT;
                    r->data.f = getNumeric(&a) * getNumeric(&b);
                } else {
                    r->type = TAG_INT;
                    r->data.i = a.data.i * b.data.i;
                }
                break;
            }
            case 0xA3: { // DIV
                if(stackPointer < 1) {
                    stackPointer++;
                    stack[stackPointer].type = TAG_INT;
                    stack[stackPointer].data.i = 0;
                    break;
                }
                Variant b = stack[stackPointer]; stackPointer--;
                Variant a = stack[stackPointer]; stackPointer--;
                stackPointer++;
                Variant* r = &stack[stackPointer];
                r->type = TAG_FLOAT;
                r->data.f = getNumeric(&a) / getNumeric(&b);
                break;
            }
            case 0xA4: { // POW
                if(stackPointer < 1) {
                    stackPointer++;
                    stack[stackPointer].type = TAG_INT;
                    stack[stackPointer].data.i = 0;
                    break;
                }
                Variant b = stack[stackPointer]; stackPointer--;
                Variant a = stack[stackPointer]; stackPointer--;
                stackPointer++;
                Variant* r = &stack[stackPointer];
                if(isFloatVariant(&a, &b)) {
                    r->type = TAG_FLOAT;
                    r->data.f = pow(getNumeric(&a), getNumeric(&b));
                } else {
                    r->type = TAG_INT;
                    r->data.i = (int32_t)pow(getNumeric(&a), getNumeric(&b));
                }
                break;
            }
            case 0xA5: { // MOD
                if(stackPointer < 1) {
                    stackPointer++;
                    stack[stackPointer].type = TAG_INT;
                    stack[stackPointer].data.i = 0;
                    break;
                }
                Variant b = stack[stackPointer]; stackPointer--;
                Variant a = stack[stackPointer]; stackPointer--;
                stackPointer++;
                Variant* r = &stack[stackPointer];
                if(isFloatVariant(&a, &b)) {
                    r->type = TAG_FLOAT;
                    r->data.f = fmod(getNumeric(&a), getNumeric(&b));
                } else {
                    r->type = TAG_INT;
                    r->data.i = a.data.i % b.data.i;
                }
                break;
            }
            case 0xA6: { // INC
                Variant* x = &stack[stackPointer];
                switch(x->type) {
                    case TAG_INT:
                        x->data.i++;
                        break;
                    case TAG_FLOAT:
                        x->data.i++;
                        break;
                    case TAG_STRING:
                        break;
                }
                break;
            }
            case 0xA7: { // DEC
                Variant* x = &stack[stackPointer];
                switch(x->type) {
                    case TAG_INT:
                        x->data.i--;
                        break;
                    case TAG_FLOAT:
                        x->data.i--;
                        break;
                    case TAG_STRING:
                        break;
                }
                break;
            }
            case 0xA8: { // INCV
                int value = bytecode[PC + 1];
                Variant* x = &variables[value];
                switch(x->type) {
                    case TAG_INT:
                        x->data.i--;
                        break;
                    case TAG_FLOAT:
                        x->data.i--;
                        break;
                    case TAG_STRING:
                        break;
                }
                break;
            }
            case 0xA9: { // DECV
                int value = bytecode[PC + 1];
                Variant* x = &variables[value];
                switch(x->type) {
                    case TAG_INT:
                        x->data.i--;
                        break;
                    case TAG_FLOAT:
                        x->data.i--;
                        break;
                    case TAG_STRING:
                        break;
                }
                break;
            }
            case 0xB0:
            case 0xB1:
            case 0xB2:
            case 0xB3:
            case 0xB4:
            case 0xB5: {
                Variant a, b;
                if(stackPointer < 1) {
                    a.type = TAG_INT; a.data.i = (int32_t)0xFEEDFACE;
                    b.type = TAG_INT; b.data.i = (int32_t)0xDEADBEEF;
                } else {
                    b = stack[stackPointer]; stackPointer--;
                    a = stack[stackPointer]; stackPointer--;
                }
                int falseIndex = bytecode[PC + 1];

                double av = getNumeric(&a);
                double bv = getNumeric(&b);
                int result = 0;
                switch(opcode) {
                    case 0xB0: result = av == bv; break;
                    case 0xB1: result = av >  bv; break;
                    case 0xB2: result = av <  bv; break;
                    case 0xB3: result = av >= bv; break;
                    case 0xB4: result = av <= bv; break;
                    case 0xB5: result = av != bv; break;
                }
                if(!result) {
                    PC = routineBase + falseIndex;
                    continue;
                }
                break;
            }
            case 0xC0:
            case 0xC1:
            case 0xC2:
            case 0xC3:
            case 0xC4:
            case 0xC5: {
                Variant b = stack[stackPointer]; stackPointer--;
                Variant a = stack[stackPointer]; stackPointer--;
                uint32_t falseIndex = readU32FromBytecode(bytecode, PC + 1);

                double av = getNumeric(&a);
                double bv = getNumeric(&b);
                int result = 0;
                switch(opcode) {
                    case 0xC0: result = av == bv; break;
                    case 0xC1: result = av >  bv; break;
                    case 0xC2: result = av <  bv; break;
                    case 0xC3: result = av >= bv; break;
                    case 0xC4: result = av <= bv; break;
                    case 0xC5: result = av != bv; break;
                }
                if(!result) {
                    PC = routineBase + (int)falseIndex;
                    continue;
                }
                break;
            }
            case 0xAA: {
                int strCount = stack[stackPointer].data.i; stackPointer--;

                size_t totalLen = 0;
                for(int i = 0; i < strCount; i++) {
                    totalLen += strlen(stack[stackPointer - i].data.str);
                }

                memset(joinBuffer, 0, sizeof(joinBuffer));
                char *dst = joinBuffer;

                for(int i = strCount - 1; i >= 0; i--) {
                    char *src = stack[stackPointer-i].data.str;
                    while(*src)
                        *dst++ = *src++;
                }

                *dst = '\0';

                stackPointer -= strCount;

                stackPointer++;
                stack[stackPointer].type = TAG_STRING;
                stack[stackPointer].data.str = joinBuffer;

                break;
            }
            case 0xAB: { // CPY
                int value = bytecode[PC + 1];
                variables[value] = stack[stackPointer];
                break;
            }
            case 0xDE: {
                if(stackPointer < 0) return -1;
                Variant ptrVar = stack[stackPointer];
                stackPointer--;

                if(ptrVar.type != TAG_INT) {
                    send_uart("Invalid dereference\n");
                    return -1;
                }

                int ptr = ptrVar.data.i;
                if(ptr < 0 || ptr >= g_variableCount) {
                    send_uart("Invalid dereference\n");
                    return -1;
                }

                stackPointer++;
                stack[stackPointer] = variables[ptr];
                break;
            }
            case 0xFF:
                halt = 1;
                break;
            default:
                send_uart("Invalid opcode\n");
                return -1;
        }
        if(halt) break;
        PC += offset;
    }
    return 0;
}

int main() {
    stdio_init_all();
    sleep_ms(2000);

    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    srand(time_us_32());

    send_uart("Loading...\n");

    int result = loadFromFlash(program, programSize);
    if(result != 0) {
        send_uart("Load failed\n");
        while(1);
    }

    send_uart("Executing...\n");
    execute(g_bytecode, g_bytecodeSize, (const char**)g_stringPool, g_stringPoolSize);
    send_uart("Done\n");

    while(1);
}