#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include <avr/io.h>
#include <util/delay.h>

PROGMEM const uint8_t program[] = {
    0xfe, 0xfd, 0x2d, 0x00, 0x00, 0x00, 0x03, 0x02, 0x00, 0x04, 0xd0, 0x03, 0x02, 0x00, 0x03, 0x02, 
    0x01, 0x04, 0xd1, 0x03, 0x02, 0x00, 0x03, 0x02, 0x01, 0x04, 0xd2, 0x03, 0x02, 0x02, 0x04, 0xd3, 
    0x03, 0x02, 0x00, 0x03, 0x02, 0x03, 0x04, 0xd2, 0x03, 0x02, 0x02, 0x04, 0xd3, 0x06, 0x0d, 0x00, 
    0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x2a, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x40, 0x7f, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const int programSize = 95;

#define MAX_STRINGS 16
#define MAX_STRING_LEN 64
#define MAX_CONST_POOL 32

#define BAUD 9600
#define UBRR_VALUE ((F_CPU / 16 / BAUD) - 1)

static uint8_t* g_bytecode = NULL;
static int g_bytecodeSize = 0;

static char* g_stringPool[MAX_STRINGS];
static int g_stringPoolSize = 0;

static float g_constPool[MAX_CONST_POOL];
static int g_constPoolSize = 0;

static int g_variableCount = 0;

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

float getNumeric(const Variant* v) {
  if (v->type == TAG_FLOAT) return v->data.f;
  return (float)v->data.i;
}

int isFloatVariant(const Variant* a, const Variant* b) {
  return a->type == TAG_FLOAT || b->type == TAG_FLOAT;
}

void uart_init() {
  UBRR0H = (uint8_t)(UBRR_VALUE >> 8);
  UBRR0L = (uint8_t)UBRR_VALUE;

  UCSR0B = (1 << TXEN0) | (1 << RXEN0);

  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_putchar(char c) {
  while (!(UCSR0A & (1 << UDRE0)));
  UDR0 = c;
}

void uart_puts(const char* s) {
  while (*s) {
    uart_putchar(*s++);
  }
}

char uart_getchar(void) {
  while (!(UCSR0A & (1 << RXC0)));
  return UDR0;
}

void send_uart(const char* message) {
  uart_puts(message);
  printf("%s", message);
}

void uart_put_variant(const Variant* v) {
  char buf[32];
  if (v->type == TAG_STRING) {
    send_uart(v->data.str);
    return;
  } else if (v->type == TAG_FLOAT) {
    snprintf(buf, sizeof(buf), "%f", v->data.f);
  } else {
    snprintf(buf, sizeof(buf), "%ld", (long)v->data.i);
  }
  send_uart(buf);
}

void uart_readline(char* buffer, int maxLen) {
  int i = 0;

  while (i < maxLen - 1) {
    char c = uart_getchar();

    if (c == '\r' || c == '\n')
      break;

    buffer[i++] = c;
    uart_putchar(c);
  }

  buffer[i] = '\0';

  uart_putchar('\r');
  uart_putchar('\n');
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

int loadFromFlash(const uint8_t* data, int dataSize) {
    int pos = 0;

    uint8_t sig0 = readByte(data, &pos);
    uint8_t sig1 = readByte(data, &pos);

    bool isV2 = (sig0 == 0xFE && sig1 == 0xFB);
    bool isV3 = (sig0 == 0xFE && sig1 == 0xFC);
    bool isV4 = (sig0 == 0xFE && sig1 == 0xFD);

    if (!isV2 && !isV3 && !isV4) {
        send_uart("Invalid signature\n");

        if (sig0 == 0xFE && sig1 == 0xFA) {
            send_uart("v1 Precompiled Lumen binaries are not compatible with v2+ Lumen runtime\n");
        }

        return -1;
    }

    int bcSize = readInt(data, &pos);

    if (g_bytecode)
        free(g_bytecode);

    g_bytecode = (uint8_t*)malloc(bcSize);

    if (!g_bytecode) {
        send_uart("malloc failed\n");
        return -1;
    }

    g_bytecodeSize = bcSize;

    for (int i = 0; i < bcSize; i++) {
        g_bytecode[i] = readByte(data, &pos);
    }

    int spSize = readInt(data, &pos);

    for (int i = 0; i < g_stringPoolSize; i++) {
        if (g_stringPool[i]) {
            free(g_stringPool[i]);
            g_stringPool[i] = NULL;
        }
    }

    g_stringPoolSize = spSize;

    for (int i = 0; i < spSize; i++) {
        int len = readInt(data, &pos);

        g_stringPool[i] = (char*)malloc(len + 1);

        if (!g_stringPool[i])
            return -1;

        for (int j = 0; j < len; j++) {
            g_stringPool[i][j] = (char)readByte(data, &pos);
        }

        g_stringPool[i][len] = '\0';
    }

    int cpSize = readInt(data, &pos);

    g_constPoolSize = cpSize;

    if (isV3 || isV4) {
        for (int i = 0; i < cpSize; i++) {
            g_constPool[i] = readfloat(data, &pos);
        }
    } else {
        for (int i = 0; i < cpSize; i++) {
            g_constPool[i] = (float)readInt(data, &pos);
        }
    }

    g_variableCount = readInt(data, &pos);

    return 0;
}

static uint32_t readU32FromBytecode(const uint8_t* bytecode, int pos) {
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
  float value = strtod(buffer, &end);

  if (*end != '\0') {
    send_uart("Invalid value!\n");
    value = 0;
  }

  variables[varRef->data.i].type = TAG_INT;
  variables[varRef->data.i].data.i = (int32_t)value;
}

void fn_inputStr(Variant stack[16], Variant variables[16], int* sp) {
  Variant* varRef = &stack[*sp];
  (*sp)--;

  char buffer[64];
  uart_readline(buffer, sizeof(buffer));

  char* str = malloc(strlen(buffer) + 1);

  if (!str)
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

void fn_int2str(Variant stack[16], Variant variables[16], int* sp) {
  Variant* varRef = &stack[*sp];
  (*sp)--;

  Variant* value = &stack[*sp];
  (*sp)--;

  int32_t integer = 0;

  if (value->type == TAG_INT) integer = value->data.i;

  char buf[12];
  snprintf(buf, sizeof(buf), "%ld", (long)integer);

  char* str = malloc(strlen(buf) + 1);

  if (!str)
    return;

  strcpy(str, buf);

  variables[varRef->data.i].type = TAG_STRING;
  variables[varRef->data.i].data.str = str;
}

void fn_str2float(Variant stack[16], Variant variables[16], int* sp) {
  Variant* varRef = &stack[*sp];
  (*sp)--;

  Variant* value = &stack[*sp];
  (*sp)--;

  int isStr = value->type == TAG_STRING;

  char* end;
  errno = 0;

  float val = isStr ? strtod(value->data.str, &end) : 0.0;

  if (isStr && end == value->data.str) {
    val = 0.0;
  } else if (errno == ERANGE) {
    val = 0.0;
  }

  variables[varRef->data.i].type = TAG_FLOAT;
  variables[varRef->data.i].data.f = val;
}

void fn_float2str(Variant stack[16], Variant variables[16], int* sp) {
  Variant* varRef = &stack[*sp];
  (*sp)--;

  Variant* value = &stack[*sp];
  (*sp)--;

  float num = 0.0;
  if (value->type == TAG_FLOAT) num = value->data.f;
  else if (value->type == TAG_INT) num = (float)value->data.i;

  char buf[32];
  snprintf(buf, sizeof(buf), "%f", num);

  char* str = malloc(strlen(buf) + 1);

  if (!str)
    return;

  strcpy(str, buf);

  variables[varRef->data.i].type = TAG_STRING;
  variables[varRef->data.i].data.str = str;
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

void sleepUs(uint16_t us) {
    while (us--) {
        _delay_us(1);
    }
}

NativeFn funcTable[] = {
  NULL,          
  fn_println,    
  fn_print,      
  fn_inputInt,   
  fn_inputStr,   
  fn_str2int,    
  fn_int2str,    
  fn_str2float,  
  fn_float2str,  
};

#define BASE_FUNC_COUNT (sizeof(funcTable) / sizeof(funcTable[0]))

NativeFn customFuncTable[] = {
  fn_gpioInit,      
  fn_gpioSetDir,    
  fn_gpioPut,       
  fn_sleepMs,       
  fn_gpioGet,       
  fn_gpioPullUp,    
  fn_gpioPullDown,
  fn_sleepUs  
};

#define CUSTOM_FUNC_COUNT (sizeof(customFuncTable) / sizeof(customFuncTable[0]))

int execute(
  const uint8_t* bytecode,
  const int bytecodeSize,
  const char* stringPool[],
  const int stringPoolSize) {
  Variant variables[16];
  Variant stack[16];
  CallFrame pcStack[8];
  int stackPointer = -1;
  int pcStackPointer = -1;
  int PC = 0;
  int routineBase = 0;
  int halt = 0;

  while (1) {
    int opcode = bytecode[PC];
    int offset = getOpCodeOffset(opcode);
    switch (opcode) {
      case 0x01:
        {
          int addr = bytecode[PC + 1];
          pcStackPointer++;
          pcStack[pcStackPointer].returnPC = PC + offset;
          pcStack[pcStackPointer].routineBase = routineBase;
          routineBase = addr;
          PC = addr;
          continue;
        }
      case 0xFE:
        {
          if (pcStackPointer < 0) return -1;
          CallFrame frame = pcStack[pcStackPointer--];
          routineBase = frame.routineBase;
          PC = frame.returnPC;
          continue;
        }
      case 0x02:
        {
          int varIndex = bytecode[PC + 1];
          if (stackPointer < 0) return -1;
          variables[varIndex].type = stack[stackPointer].type;
          switch (stack[stackPointer].type) {
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
      case 0x03:
        {
          int dataType = bytecode[PC + 1];
          int value = bytecode[PC + 2];
          stackPointer++;
          Variant* v = &stack[stackPointer];
          switch (dataType) {
            case 0x01:
              v->type = TAG_STRING;
              v->data.str = (char*)stringPool[value];
              break;
            case 0x02: 
              v->type = TAG_INT;
              v->data.i = (int32_t)g_constPool[value]; 
              break;
            case 0x03: 
              {
                Variant* var = &variables[value];
                v->type = var->type;
                if (var->type == TAG_INT)
                  v->data.i = var->data.i;
                else if (var->type == TAG_FLOAT)
                  v->data.f = var->data.f;
                else
                  v->data.str = var->data.str;
                break;
              }
            case 0x04: 
              v->type = TAG_INT;
              v->data.i = (int32_t)value;
              break;
            case 0x05: 
              v->type = TAG_FLOAT;
              v->data.f = g_constPool[value];
              break;
          }
          break;
        }
      case 0x04:
        {
          int addr = bytecode[PC + 1];

          if (addr >= 0xD0 && addr <= 0xFF) {
            int customIndex = addr - 0xD0;
            if (customIndex < 0 || (unsigned)customIndex >= CUSTOM_FUNC_COUNT) {
              send_uart("Invalid native call\n");
              return -1;
            }
            if (customFuncTable[customIndex])
              customFuncTable[customIndex](stack, variables, &stackPointer);
          } else {
            if (addr < 0 || (unsigned)addr >= BASE_FUNC_COUNT) {
              send_uart("Invalid native call\n");
              return -1;
            }
            if (funcTable[addr])
              funcTable[addr](stack, variables, &stackPointer);
          }

          break;
        }
      case 0x05:
        {
          int newPC = bytecode[PC + 1];
          PC = routineBase + newPC;
          continue;
        }
      case 0x06:
        {  
          uint32_t addr = readU32FromBytecode(bytecode, PC + 1);
          PC = routineBase + (int)addr;
          continue;
        }
      case 0x07:
        {  
          uint32_t target = readU32FromBytecode(bytecode, PC + 1);
          pcStackPointer++;
          pcStack[pcStackPointer].returnPC = PC + offset;
          pcStack[pcStackPointer].routineBase = routineBase;
          routineBase = (int)target;
          PC = (int)target;
          continue;
        }
      case 0xA0:
        {  
          Variant b = stack[stackPointer];
          stackPointer--;
          Variant a = stack[stackPointer];
          stackPointer--;
          stackPointer++;
          Variant* r = &stack[stackPointer];
          if (isFloatVariant(&a, &b)) {
            r->type = TAG_FLOAT;
            r->data.f = getNumeric(&a) + getNumeric(&b);
          } else {
            r->type = TAG_INT;
            r->data.i = a.data.i + b.data.i;
          }
          break;
        }
      case 0xA1:
        {  
          Variant b = stack[stackPointer];
          stackPointer--;
          Variant a = stack[stackPointer];
          stackPointer--;
          stackPointer++;
          Variant* r = &stack[stackPointer];
          if (isFloatVariant(&a, &b)) {
            r->type = TAG_FLOAT;
            r->data.f = getNumeric(&a) - getNumeric(&b);
          } else {
            r->type = TAG_INT;
            r->data.i = a.data.i - b.data.i;
          }
          break;
        }
      case 0xA2:
        {  
          Variant b = stack[stackPointer];
          stackPointer--;
          Variant a = stack[stackPointer];
          stackPointer--;
          stackPointer++;
          Variant* r = &stack[stackPointer];
          if (isFloatVariant(&a, &b)) {
            r->type = TAG_FLOAT;
            r->data.f = getNumeric(&a) * getNumeric(&b);
          } else {
            r->type = TAG_INT;
            r->data.i = a.data.i * b.data.i;
          }
          break;
        }
      case 0xA3:
        {  
          Variant b = stack[stackPointer];
          stackPointer--;
          Variant a = stack[stackPointer];
          stackPointer--;
          stackPointer++;
          Variant* r = &stack[stackPointer];
          r->type = TAG_FLOAT;
          r->data.f = getNumeric(&a) / getNumeric(&b);
          break;
        }
      case 0xA4:
        {  
          Variant b = stack[stackPointer];
          stackPointer--;
          Variant a = stack[stackPointer];
          stackPointer--;
          stackPointer++;
          Variant* r = &stack[stackPointer];
          if (isFloatVariant(&a, &b)) {
            r->type = TAG_FLOAT;
            r->data.f = pow(getNumeric(&a), getNumeric(&b));
          } else {
            r->type = TAG_INT;
            r->data.i = (int32_t)pow(getNumeric(&a), getNumeric(&b));
          }
          break;
        }
      case 0xA5:
        {  
          Variant b = stack[stackPointer];
          stackPointer--;
          Variant a = stack[stackPointer];
          stackPointer--;
          stackPointer++;
          Variant* r = &stack[stackPointer];
          if (isFloatVariant(&a, &b)) {
            r->type = TAG_FLOAT;
            r->data.f = fmod(getNumeric(&a), getNumeric(&b));
          } else {
            r->type = TAG_INT;
            r->data.i = a.data.i % b.data.i;
          }
          break;
        }
      case 0xB0:
      case 0xB1:
      case 0xB2:
      case 0xB3:
      case 0xB4:
      case 0xB5:
        {
          Variant b = stack[stackPointer];
          stackPointer--;
          Variant a = stack[stackPointer];
          stackPointer--;
          int falseIndex = bytecode[PC + 1];

          float av = getNumeric(&a);
          float bv = getNumeric(&b);
          int result = 0;
          switch (opcode) {
            case 0xB0: result = av == bv; break;
            case 0xB1: result = av > bv; break;
            case 0xB2: result = av < bv; break;
            case 0xB3: result = av >= bv; break;
            case 0xB4: result = av <= bv; break;
            case 0xB5: result = av != bv; break;
          }
          if (!result) {
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
      case 0xC5:
        {
          Variant b = stack[stackPointer];
          stackPointer--;
          Variant a = stack[stackPointer];
          stackPointer--;
          uint32_t falseIndex = readU32FromBytecode(bytecode, PC + 1);

          float av = getNumeric(&a);
          float bv = getNumeric(&b);
          int result = 0;
          switch (opcode) {
            case 0xC0: result = av == bv; break;
            case 0xC1: result = av > bv; break;
            case 0xC2: result = av < bv; break;
            case 0xC3: result = av >= bv; break;
            case 0xC4: result = av <= bv; break;
            case 0xC5: result = av != bv; break;
          }
          if (!result) {
            PC = routineBase + (int)falseIndex;
            continue;
          }
          break;
        }
      case 0xAA:
        {
          int strCount = stack[stackPointer].data.i;
          stackPointer--;

          size_t totalLen = 0;
          for (int i = 0; i < strCount; i++) {
            totalLen += strlen(stack[stackPointer - i].data.str);
          }

          char* result = malloc(totalLen + 1);
          if (!result) return -1;

          char* dst = result;

          for (int i = strCount - 1; i >= 0; i--) {
            char* src = stack[stackPointer - i].data.str;
            while (*src)
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
    if (halt) break;
    PC += offset;
  }
  return 0;
}

int main() {
  _delay_ms(1000);
  uart_init();

  memset(g_stringPool, 0, sizeof(g_stringPool));

  send_uart("Loading...\n");

  int result = loadFromFlash(program, programSize);
  if (result != 0) {
    send_uart("Load failed\n");
    while (1)
      ;
  }

  send_uart("Executing...\n");
  execute(g_bytecode, g_bytecodeSize, (const char**)g_stringPool, g_stringPoolSize);
  send_uart("Done\n");

  while (1)
    ;
}