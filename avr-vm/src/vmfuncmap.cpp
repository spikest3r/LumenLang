#include "includes.h"
#include "program.h"
#include "vm.h"
#include "vmfuncmap.h"
#include "uart.h"
#include "vmnativefuncmap.h"
#include "helpers.h"

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

static char buf_inputStr[64];

void fn_inputStr(Variant stack[16], Variant variables[16], int* sp) {
  Variant* varRef = &stack[*sp];
  (*sp)--;

  char buffer[64];
  uart_readline(buffer, sizeof(buffer));

  char* str = buf_inputStr;

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

static char buf_i2s[12];

void fn_int2str(Variant stack[16], Variant variables[16], int* sp) {
  Variant* varRef = &stack[*sp];
  (*sp)--;

  Variant* value = &stack[*sp];
  (*sp)--;

  int32_t integer = 0;

  if (value->type == TAG_INT) integer = value->data.i;

  char buf[12];
  snprintf(buf, sizeof(buf), "%ld", (long)integer);

  char* str = buf_i2s;

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

static char buf_f2s[32];

void fn_float2str(Variant stack[16], Variant variables[16], int* sp) {
  Variant* varRef = &stack[*sp];
  (*sp)--;

  Variant* value = &stack[*sp];
  (*sp)--;

  float num = 0.0f;

  if (value->type == TAG_FLOAT)
    num = value->data.f;
  else if (value->type == TAG_INT)
    num = (float)value->data.i;

  char buf[32];

  int32_t whole = (int32_t)num;
  int32_t frac = (int32_t)((num - whole) * 100);

  if (frac < 0)
    frac = -frac;

  snprintf(buf, sizeof(buf), "%ld.%02ld",
           (long)whole,
           (long)frac);

  char* str = buf_f2s;

  if (!str)
    return;

  strcpy(str, buf);

  variables[varRef->data.i].type = TAG_STRING;
  variables[varRef->data.i].data.str = str;
}