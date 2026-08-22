#include "includes.h"
#include "program.h"
#include "vm.h"
#include "vmfuncmap.h"
#include "uart.h"
#include "vmnativefuncmap.h"
#include "helpers.h"
#include <ctype.h>

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

// capabilities

static const char* supportedCapabilities[] = {
    "random"
};

#define SUPPORTED_CAPABILITY_COUNT (sizeof(supportedCapabilities) / sizeof(supportedCapabilities[0]))

void fn_assertCapability(Variant stack[16], Variant variables[16], int* sp) {
    // last value on stack is capability name

    Variant* capability = &stack[*sp];
    if(capability->type != TAG_STRING) {
        send_uart("assertCapability failed: invalid type\n");
        halt = 1;
        return;
    }
    (*sp)--;

    for (unsigned int i = 0; i < SUPPORTED_CAPABILITY_COUNT; i++) {
        if (strcmp(capability->data.str, supportedCapabilities[i]) == 0) {
            // capability present, proceed with execution
            return;
        }
    }

    char buffer[64];
    snprintf(buffer, sizeof(buffer), "assertCapability failed: capability '%s' is not supported on this platform\n", capability->data.str);
    send_uart(buffer);
    halt = 1;
}

void fn_unsupportedCapability(Variant stack[16], Variant variables[16], int* sp) {
    send_uart("capability not supported on this platform\n");
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

    float val = (float)rand() / ((float)RAND_MAX + 1.0f);

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

    int32_t range = max - min + 1;
    int32_t val = (range > 0) ? (min + (rand() % range)) : min;

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

static char buf_substr[MAX_STRING_LEN + 1];

void fn_substr(Variant stack[16], Variant variables[16], int* sp) {
    Variant* varRef = &stack[*sp];
    (*sp)--;

    Variant* lenArg = &stack[*sp];
    (*sp)--;

    Variant* startArg = &stack[*sp];
    (*sp)--;

    Variant* value = &stack[*sp];
    (*sp)--;

    buf_substr[0] = '\0';

    if (value->type == TAG_STRING) {
        char* str = value->data.str;
        int32_t strLen = (int32_t)strlen(str);
        int32_t start = startArg->data.i;
        int32_t len = lenArg->data.i;

        if (start >= 0 && start < strLen) {
            int32_t avail = strLen - start;
            if (len > avail) len = avail;
            if (len > MAX_STRING_LEN) len = MAX_STRING_LEN;
            if (len > 0) {
                memcpy(buf_substr, str + start, len);
            }
            buf_substr[len > 0 ? len : 0] = '\0';
        }
    }

    variables[varRef->data.i].type = TAG_STRING;
    variables[varRef->data.i].data.str = buf_substr;
}

void fn_strfind(Variant stack[16], Variant variables[16], int* sp) {
    Variant* varRef = &stack[*sp];
    (*sp)--;

    Variant* needleArg = &stack[*sp];
    (*sp)--;

    Variant* value = &stack[*sp];
    (*sp)--;

    int32_t result = -1;

    if (value->type == TAG_STRING && needleArg->type == TAG_STRING) {
        char* found = strstr(value->data.str, needleArg->data.str);
        if (found != NULL) {
            result = (int32_t)(found - value->data.str);
        }
    }

    variables[varRef->data.i].type = TAG_INT;
    variables[varRef->data.i].data.i = result;
}

static char buf_strcase[MAX_STRING_LEN + 1];

void fn_strcase(Variant stack[16], Variant variables[16], int* sp) {
    Variant* varRef = &stack[*sp];
    (*sp)--;

    Variant* upperFlagArg = &stack[*sp];
    (*sp)--;

    Variant* value = &stack[*sp];
    (*sp)--;

    buf_strcase[0] = '\0';

    if (value->type == TAG_STRING) {
        int upperFlag = upperFlagArg->data.i;
        char* src = value->data.str;
        int32_t i = 0;
        for (; i < MAX_STRING_LEN && src[i] != '\0'; i++) {
            buf_strcase[i] = upperFlag ? toupper((unsigned char)src[i]) : tolower((unsigned char)src[i]);
        }
        buf_strcase[i] = '\0';
    }

    variables[varRef->data.i].type = TAG_STRING;
    variables[varRef->data.i].data.str = buf_strcase;
}

static char buf_trim[MAX_STRING_LEN + 1];

void fn_trim(Variant stack[16], Variant variables[16], int* sp) {
    Variant* varRef = &stack[*sp];
    (*sp)--;

    Variant* value = &stack[*sp];
    (*sp)--;

    buf_trim[0] = '\0';

    if (value->type == TAG_STRING) {
        char* str = value->data.str;
        int32_t len = (int32_t)strlen(str);

        int32_t start = 0;
        while (start < len && isspace((unsigned char)str[start])) start++;

        int32_t end = len - 1;
        while (end >= start && isspace((unsigned char)str[end])) end--;

        int32_t trimmedLen = (end >= start) ? (end - start + 1) : 0;
        if (trimmedLen > MAX_STRING_LEN) trimmedLen = MAX_STRING_LEN;

        if (trimmedLen > 0) {
            memcpy(buf_trim, str + start, trimmedLen);
        }
        buf_trim[trimmedLen] = '\0';
    }

    variables[varRef->data.i].type = TAG_STRING;
    variables[varRef->data.i].data.str = buf_trim;
}