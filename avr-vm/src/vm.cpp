#include "includes.h"
#include "program.h"
#include "vm.h"
#include "vmfuncmap.h"
#include "uart.h"
#include "vmnativefuncmap.h"
#include "helpers.h"

typedef void (*NativeFn)(Variant stack[16], Variant variables[16], int* sp);

static char buf[32];

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
  fn_sleepUs,
  fn_oneWireReset,
  fn_oneWireWriteByte,
  fn_oneWireReadByte,
  fn_shiftLeft,
  fn_shiftRight,
  fn_or
};

#define CUSTOM_FUNC_COUNT (sizeof(customFuncTable) / sizeof(customFuncTable[0]))

int execute(
  const uint8_t* bytecode,
  const int bytecodeSize,
  const char* stringPool[],
  const int stringPoolSize,
  const float* g_constPool)
{
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

          if(totalLen > 32) {
            send_uart("Concat result is longer than 32!\n");
            return -1;
          }

          char* dst = buf;

          for (int i = strCount - 1; i >= 0; i--) {
            char* src = stack[stackPointer - i].data.str;
            while (*src)
              *dst++ = *src++;
          }

          *dst = '\0';

          stackPointer -= strCount;

          stackPointer++;
          stack[stackPointer].type = TAG_STRING;
          stack[stackPointer].data.str = buf;

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