#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
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

static int g_constPool[MAX_CONST_POOL];
static int g_constPoolSize = 0;

static int g_variableCount = 0;

typedef enum {
    TAG_INT = 2,
    TAG_STRING = 1
} TypeTag;

typedef struct {
    TypeTag type;
    union {
        int32_t i;
        char* str;
    } data;
} Variant;

void send_uart(const char* message) {
    uart_puts(UART_ID, message);
    printf("%s", message);
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

int loadFromFlash(const uint8_t* data, int dataSize) {
    int pos = 0;

    uint8_t sig0 = data[pos++];
    uint8_t sig1 = data[pos++];
    if (sig0 != 0xFE || sig1 != 0xFB) {
        send_uart("Invalid signature\n");
        if (sig1 == 0xFA) {
            send_uart("v1 Precompiled Lumen binaries are not compatible with v2 Lumen runtime\n");
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
    for (int i = 0; i < cpSize; i++) {
        g_constPool[i] = readInt(data, &pos);
    }

    // variable count
    g_variableCount = readInt(data, &pos);

    return 0;
}

int getOpCodeOffset(int opcode) {
    switch(opcode) {
        case 0x03:
            return 3;
        case 0x04:
        case 0x02:
        case 0xB0: // ==
        case 0xB1: // >
        case 0xB2: // <
        case 0xB3: // >=
        case 0xB4: // <=
        case 0xB5: // != 
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
    }
    return 0;
}

typedef void (*NativeFn)(Variant stack[16], Variant variables[16], int* sp);

void fn_println(Variant stack[16], Variant variables[16], int* sp) {
    Variant* arg = &stack[*sp];
    (*sp)--;
    if(arg->type == TAG_STRING) {
        send_uart(arg->data.str);
        send_uart("\n");
    } else {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d\n", (long)arg->data.i);
        send_uart(buf);
    }
}

void fn_print(Variant stack[16], Variant variables[16], int* sp) {
    Variant* arg = &stack[*sp];
    (*sp)--;
    if(arg->type == TAG_STRING) {
        send_uart(arg->data.str);
    } else {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d\n", (int)arg->data.i);
        send_uart(buf);
    }
}

void fn_inputInt(Variant stack[16], Variant variables[16], int* sp) {
    Variant* varRef = &stack[*sp];
    (*sp)--;

    char buffer[32];
    uart_readline(buffer, sizeof(buffer));

    int32_t value = 0;

    char* end;
    value = strtoll(buffer, &end, 10);

    if(*end != '\0') {
        send_uart("Invalid value!\n");
        value = 0;
    }

    variables[varRef->data.i].type = TAG_INT;
    variables[varRef->data.i].data.i = value;
}

void fn_inputStr(Variant stack[16], Variant variables[16], int* sp) {
    Variant* varRef = &stack[*sp];
    (*sp)--;

    char buffer[64];
    uart_readline(buffer, sizeof(buffer));

    char* str = malloc(strlen(buffer) + 1);

    if(!str)
        return;

    strcpy(str, buffer);

    variables[varRef->data.i].type = TAG_STRING;
    variables[varRef->data.i].data.str = str;
}

void fn_str2int(Variant stack[16], Variant variables[16], int* sp) {
    Variant* varRef = &stack[*sp];
    (*sp)--;

    Variant* value = &stack[*sp];
    (*sp)--;

    bool isStr = variables[varRef->data.i].type == TAG_STRING;

    char* str;
    if(isStr)
        str = variables[varRef->data.i].data.str;

    char* end;
    errno = 0;

    long val = isStr ? strtol(str, &end, 10) : 0;

    if (end == str) {
        val = 0;
    } else if (errno == ERANGE) {
        val = 0;
    } else if(!isStr) {
        val = 0;
    }

    variables[varRef->data.i].type = TAG_STRING;
    variables[varRef->data.i].data.i = (int)val;
} 

void fn_int2str(Variant stack[16], Variant variables[16], int* sp) {
    Variant* varRef = &stack[*sp];
    (*sp)--;

    Variant* value = &stack[*sp];
    (*sp)--;

    int32_t integer = 0;

    if(value->type == TAG_INT) integer = value->data.i;

    char buf[12];
    snprintf(buf, sizeof(buf), "%d", integer);

    char* str = malloc(strlen(buf) + 1);

    if(!str)
        return;

    strcpy(str, buf);

    variables[varRef->data.i].type = TAG_STRING;
    variables[varRef->data.i].data.str = str;
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

NativeFn funcTable[] = {
    NULL,
    fn_println,
    fn_print,
    fn_inputInt,
    fn_str2int,
    fn_int2str,
    fn_gpioInit,
    fn_gpioSetDir,
    fn_gpioPut,
    fn_sleepMs,
    fn_gpioGet,
    fn_gpioPullUp,
    fn_gpioPullDown
};

int execute(
    const uint8_t* bytecode,
    const int bytecodeSize,
    const char* stringPool[],
    const int stringPoolSize
) {
    Variant variables[16];
    Variant stack[16];
    int pcStack[8];
    int stackPointer = -1;
    int pcStackPointer = -1;
    int PC = 0;
    int halt = 0;

    while(1) {
        int opcode = bytecode[PC];
        int offset = getOpCodeOffset(opcode);
        switch(opcode) {
            case 0x01: {
                int addr = bytecode[PC + 1];
                pcStack[++pcStackPointer] = PC + offset;
                PC = addr;
                continue;
            }
            case 0xFE: {
                if(pcStackPointer < 0) return -1;
                PC = pcStack[pcStackPointer--];
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
                        else
                            v->data.str = var->data.str;
                        break;
                    }
                    case 0x04: {
                        v->type = TAG_INT;
                        v->data.i = (int32_t)value;
                        break;
                    }
                }
                break;
            }
            case 0x04: {
                int addr = bytecode[PC + 1];
                int index;

                if(addr >= 0xD0 && addr <= 0xFF) {
                    index = (addr - 0xD0) + 6;
                }
                else {
                    index = addr;
                }

                if(index < 0 || index >= sizeof(funcTable)/sizeof(funcTable[0])) {
                    send_uart("Invalid native call\n");
                    return -1;
                }

                if(funcTable[index])
                    funcTable[index](stack, variables, &stackPointer);

                break;
            }
            case 0x05: {
                PC = bytecode[PC + 1];
                continue;
            }
            case 0xA0: {
                int32_t b = stack[stackPointer].data.i; stackPointer--;
                int32_t a = stack[stackPointer].data.i; stackPointer--;
                stackPointer++;
                stack[stackPointer].type = TAG_INT;
                stack[stackPointer].data.i = a + b;
                break;
            }
            case 0xA1: {
                int32_t b = stack[stackPointer].data.i; stackPointer--;
                int32_t a = stack[stackPointer].data.i; stackPointer--;
                stackPointer++;
                stack[stackPointer].type = TAG_INT;
                stack[stackPointer].data.i = a - b;
                break;
            }
            case 0xA2: {
                int32_t b = stack[stackPointer].data.i; stackPointer--;
                int32_t a = stack[stackPointer].data.i; stackPointer--;
                stackPointer++;
                stack[stackPointer].type = TAG_INT;
                stack[stackPointer].data.i = a * b;
                break;
            }
            case 0xA3: {
                int32_t b = stack[stackPointer].data.i; stackPointer--;
                int32_t a = stack[stackPointer].data.i; stackPointer--;
                stackPointer++;
                stack[stackPointer].type = TAG_INT;
                stack[stackPointer].data.i = a / b;
                break;
            }
            case 0xA4: {
                int32_t b = stack[stackPointer].data.i; stackPointer--;
                int32_t a = stack[stackPointer].data.i; stackPointer--;
                stackPointer++;
                stack[stackPointer].type = TAG_INT;
                stack[stackPointer].data.i = (int32_t)pow((double)a, (double)b);
                break;
            }
            case 0xA5: {
                int32_t b = stack[stackPointer].data.i; stackPointer--;
                int32_t a = stack[stackPointer].data.i; stackPointer--;
                stackPointer++;
                stack[stackPointer].type = TAG_INT;
                stack[stackPointer].data.i = a % b;
                break;
            }
            case 0xB0:
            case 0xB1:
            case 0xB2:
            case 0xB3:
            case 0xB4:
            case 0xB5: {
                int32_t b = stack[stackPointer].data.i; stackPointer--;
                int32_t a = stack[stackPointer].data.i; stackPointer--;
                int falseIndex = bytecode[PC + 1];
                int result = 0;
                switch(opcode) {
                    case 0xB0: result = a == b; break;
                    case 0xB1: result = a >  b; break;
                    case 0xB2: result = a <  b; break;
                    case 0xB3: result = a >= b; break;
                    case 0xB4: result = a <= b; break;
                    case 0xB5: result = a != b; break;
                }
                if(!result) {
                    PC = falseIndex;
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

                char* result = malloc(totalLen + 1);
                if(!result) return -1;

                char *dst = result;

                for(int i = strCount - 1; i >= 0; i--) {
                    char *src = stack[stackPointer-i].data.str;
                    while(*src)
                        *dst++ = *src++;
                }

                *dst = '\0';

                stackPointer -= strCount;

                stackPointer++;
                stack[stackPointer].type = TAG_STRING;
                stack[stackPointer].data.str = result;

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