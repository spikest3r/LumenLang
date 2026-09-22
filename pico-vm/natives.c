/*
 * Native functions (opcode 0x04).
 *
 *   0x01-0x08  base functions           (same IDs / semantics as desktop)
 *   0xA0-0xAD  capability functions     (same IDs as desktop; FS/HTTP unsupported here)
 *   0xD0-0xD6  Pico-only GPIO helpers
 *
 * Calling convention follows the desktop VM: arguments are popped from the
 * stack (last argument on top) and the result is PUSHED back on the stack.
 * The one exception is httpRequest, which is unsupported on Pico anyway.
 */
#include "vm.h"
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "pico/stdlib.h"

/* gpioGet: 1 = push result on stack (desktop-style), 0 = legacy "&outVar" argument */
#ifndef GPIO_GET_RETURNS_VIA_STACK
#define GPIO_GET_RETURNS_VIA_STACK 1
#endif

typedef int (*NativeFn)(VM* vm);

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */
#define TRY(expr) do { if ((expr) < 0) return -1; } while (0)

static int native_fail(VM* vm, const char* msg) {
    return vm_error(vm, "Function error", msg);
}

static int pop_int(VM* vm, int64_t* out) {
    Variant v;
    TRY(vm_pop(vm, &v));
    if (v.type != TAG_INT) return native_fail(vm, "expected an integer argument");
    *out = v.data.i;
    return 0;
}

static int pop_str(VM* vm, const char** out) {
    Variant v;
    TRY(vm_pop(vm, &v));
    if (v.type != TAG_STRING || !v.data.str) return native_fail(vm, "expected a string argument");
    *out = v.data.str;
    return 0;
}

static int push_string(VM* vm, const char* s, size_t len) {
    char* p = str_alloc(len);
    if (!p) return native_fail(vm, "out of memory");
    memcpy(p, s, len);
    return vm_push(vm, v_str_heap(p));
}

static void print_variant(const Variant* v) {
    char buf[64];
    switch (v->type) {
    case TAG_STRING: send_uart(v->data.str ? v->data.str : ""); break;
    case TAG_INT:    i64_to_str(v->data.i, buf); send_uart(buf); break;
    case TAG_FLOAT:  snprintf(buf, sizeof(buf), "%g", v->data.f); send_uart(buf); break;
    }
}

static int check_pin(VM* vm, int64_t pin) {
    if (pin < 0 || pin > 29) return native_fail(vm, "invalid GPIO pin");
    return 0;
}

/* ------------------------------------------------------------------ */
/* Base functions 0x01 - 0x08                                          */
/* ------------------------------------------------------------------ */
static int fn_println(VM* vm) {
    Variant a; TRY(vm_pop(vm, &a));
    print_variant(&a);
    send_uart("\n");
    return 0;
}

static int fn_print(VM* vm) {
    Variant a; TRY(vm_pop(vm, &a));
    print_variant(&a);
    return 0;
}

static char g_inputBuf[INPUT_BUF_LEN];

static int fn_inputInt(VM* vm) {
    uart_readline(g_inputBuf, sizeof(g_inputBuf));
    char* end;
    errno = 0;
    long long v = strtoll(g_inputBuf, &end, 10);
    int64_t result = 0;
    if (end == g_inputBuf || errno == ERANGE) send_uart("Invalid value!\n");
    else result = v;
    return vm_push(vm, v_int(result));
}

static int fn_inputStr(VM* vm) {
    uart_readline(g_inputBuf, sizeof(g_inputBuf));
    return push_string(vm, g_inputBuf, strlen(g_inputBuf));
}

static int fn_str2int(VM* vm) {                 /* like std::stoi: errors halt */
    const char* s; TRY(pop_str(vm, &s));
    char* end;
    errno = 0;
    long long v = strtoll(s, &end, 10);
    if (end == s)                                  return native_fail(vm, "stoi");
    if (errno == ERANGE || v < INT_MIN || v > INT_MAX) return native_fail(vm, "stoi");
    return vm_push(vm, v_int(v));
}

static int fn_int2str(VM* vm) {
    int64_t n; TRY(pop_int(vm, &n));
    char buf[24];
    size_t len = i64_to_str(n, buf);
    return push_string(vm, buf, len);
}

static int fn_str2float(VM* vm) {               /* like std::stod, but bad input -> 0.0 */
    const char* s; TRY(pop_str(vm, &s));
    char* end;
    errno = 0;
    double v = strtod(s, &end);
    if (end == s || errno == ERANGE) v = 0.0;
    return vm_push(vm, v_float(v));
}

static int fn_float2str(VM* vm) {
    Variant a; TRY(vm_pop(vm, &a));
    double d = 0.0;
    if (a.type == TAG_FLOAT)    d = a.data.f;
    else if (a.type == TAG_INT) d = (double)a.data.i;
    char buf[64];
    int len = snprintf(buf, sizeof(buf), "%f", d);
    if (len < 0) len = 0;
    if ((size_t)len >= sizeof(buf)) len = sizeof(buf) - 1;
    return push_string(vm, buf, (size_t)len);
}

/* ------------------------------------------------------------------ */
/* Capability functions 0xA0 - 0xAD                                    */
/* ------------------------------------------------------------------ */
static int fn_assertCapability(VM* vm) {
    Variant v; TRY(vm_pop(vm, &v));
    if (v.type != TAG_STRING || !v.data.str)
        return native_fail(vm, "assertCapability failed: invalid value type");

    /* Pico supports "random" only (FS / HTTP are desktop-only) */
    if (strcmp(v.data.str, "random") == 0) return 0;

    char msg[128];
    snprintf(msg, sizeof(msg), "assertCapability failed: capability '%s' is not supported on this platform", v.data.str);
    return native_fail(vm, msg);
}

static int fn_unsupported(VM* vm) {
    return native_fail(vm, "this function is not supported on the Pico platform");
}

static int fn_randomSeed(VM* vm) {
    int64_t seed; TRY(pop_int(vm, &seed));
    srand((unsigned int)seed);
    return 0;
}

static int fn_random(VM* vm) {
    double val = (double)rand() / ((double)RAND_MAX + 1.0);
    return vm_push(vm, v_float(val));
}

static int fn_randomRange(VM* vm) {
    int64_t max, min;
    TRY(pop_int(vm, &max));
    TRY(pop_int(vm, &min));
    if (min > max) return native_fail(vm, "randomRange: min must not exceed max");

    uint64_t r    = ((uint64_t)rand() << 31) ^ (uint64_t)rand();
    uint64_t span = (uint64_t)max - (uint64_t)min + 1;      /* 0 == full 64-bit range */
    uint64_t off  = span ? (r % span) : r;
    return vm_push(vm, v_int((int64_t)((uint64_t)min + off)));
}

static int fn_strlen(VM* vm) {
    const char* s; TRY(pop_str(vm, &s));
    return vm_push(vm, v_int((int64_t)strlen(s)));
}

static int fn_substr(VM* vm) {                  /* substr(s, start, len) */
    int64_t len, start;
    const char* s;
    TRY(pop_int(vm, &len));
    TRY(pop_int(vm, &start));
    TRY(pop_str(vm, &s));

    size_t n = strlen(s), rl = 0;
    const char* res = "";
    if (start >= 0 && (uint64_t)start < n) {
        size_t avail = n - (size_t)start;
        /* negative len behaves like desktop's size_t cast: "to end of string" */
        rl  = (len < 0 || (uint64_t)len > avail) ? avail : (size_t)len;
        res = s + start;
    }
    return push_string(vm, res, rl);
}

static int fn_strfind(VM* vm) {                 /* strfind(s, needle) -> index or -1 */
    const char *needle, *s;
    TRY(pop_str(vm, &needle));
    TRY(pop_str(vm, &s));
    const char* found = strstr(s, needle);
    return vm_push(vm, v_int(found ? (int64_t)(found - s) : -1));
}

static int fn_toCase(VM* vm) {                  /* toCase(s, upperFlag) */
    int64_t upper;
    const char* s;
    TRY(pop_int(vm, &upper));
    TRY(pop_str(vm, &s));

    size_t n = strlen(s);
    char* p = str_alloc(n);
    if (!p) return native_fail(vm, "out of memory");
    for (size_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)s[i];
        p[i] = upper ? (char)toupper(c) : (char)tolower(c);
    }
    return vm_push(vm, v_str_heap(p));
}

static int fn_trim(VM* vm) {
    const char* s; TRY(pop_str(vm, &s));
    const char* ws = " \t\n\r\f\v";
    size_t n     = strlen(s);
    size_t start = strspn(s, ws);
    size_t end   = n;
    while (end > start && strchr(ws, s[end - 1])) end--;
    return push_string(vm, s + start, end - start);
}

/* ------------------------------------------------------------------ */
/* Pico GPIO 0xD0 - 0xD6                                               */
/* ------------------------------------------------------------------ */
static int fn_gpioInit(VM* vm) {
    int64_t pin; TRY(pop_int(vm, &pin)); TRY(check_pin(vm, pin));
    gpio_init((uint)pin);
    return 0;
}

static int fn_gpioSetDir(VM* vm) {              /* gpioSetDir(pin, 1=out / 0=in) */
    int64_t dir, pin;
    TRY(pop_int(vm, &dir));
    TRY(pop_int(vm, &pin)); TRY(check_pin(vm, pin));
    gpio_set_dir((uint)pin, dir == 1 ? GPIO_OUT : GPIO_IN);
    return 0;
}

static int fn_gpioPut(VM* vm) {                 /* gpioPut(pin, value) */
    int64_t val, pin;
    TRY(pop_int(vm, &val));
    TRY(pop_int(vm, &pin)); TRY(check_pin(vm, pin));
    gpio_put((uint)pin, val != 0);
    return 0;
}

static int fn_sleepMs(VM* vm) {
    int64_t ms; TRY(pop_int(vm, &ms));
    if (ms > 0) sleep_ms((uint32_t)ms);
    return 0;
}

static int fn_gpioGet(VM* vm) {
#if GPIO_GET_RETURNS_VIA_STACK
    int64_t pin; TRY(pop_int(vm, &pin)); TRY(check_pin(vm, pin));
    return vm_push(vm, v_int(gpio_get((uint)pin) ? 1 : 0));
#else
    int64_t varRef, pin;
    TRY(pop_int(vm, &varRef));
    TRY(pop_int(vm, &pin)); TRY(check_pin(vm, pin));
    if (varRef < 0 || varRef >= MAX_VARIABLES) return native_fail(vm, "invalid variable index");
    vm->variables[varRef] = v_int(gpio_get((uint)pin) ? 1 : 0);
    return 0;
#endif
}

static int fn_gpioPullUp(VM* vm) {
    int64_t pin; TRY(pop_int(vm, &pin)); TRY(check_pin(vm, pin));
    gpio_pull_up((uint)pin);
    return 0;
}

static int fn_gpioPullDown(VM* vm) {
    int64_t pin; TRY(pop_int(vm, &pin)); TRY(check_pin(vm, pin));
    gpio_pull_down((uint)pin);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Dispatch tables                                                     */
/* ------------------------------------------------------------------ */
static const NativeFn baseTable[] = {
    NULL,               /* 0x00 unused */
    fn_println,         /* 0x01 */
    fn_print,           /* 0x02 */
    fn_inputInt,        /* 0x03 */
    fn_inputStr,        /* 0x04 */
    fn_str2int,         /* 0x05 */
    fn_int2str,         /* 0x06 */
    fn_str2float,       /* 0x07 */
    fn_float2str,       /* 0x08 */
};

static const NativeFn capabilityTable[] = {
    fn_assertCapability,    /* 0xA0 */
    fn_unsupported,         /* 0xA1 openFile   (no filesystem) */
    fn_unsupported,         /* 0xA2 writeFile  */
    fn_unsupported,         /* 0xA3 readFile   */
    fn_unsupported,         /* 0xA4 closeFile  */
    fn_randomSeed,          /* 0xA5 */
    fn_random,              /* 0xA6 */
    fn_randomRange,         /* 0xA7 */
    fn_unsupported,         /* 0xA8 httpRequest (no network stack) */
    fn_strlen,              /* 0xA9 */
    fn_substr,              /* 0xAA */
    fn_strfind,             /* 0xAB */
    fn_toCase,              /* 0xAC */
    fn_trim,                /* 0xAD */
};

static const NativeFn customTable[] = {
    fn_gpioInit,        /* 0xD0 */
    fn_gpioSetDir,      /* 0xD1 */
    fn_gpioPut,         /* 0xD2 */
    fn_sleepMs,         /* 0xD3 */
    fn_gpioGet,         /* 0xD4 */
    fn_gpioPullUp,      /* 0xD5 */
    fn_gpioPullDown,    /* 0xD6 */
};

#define COUNT(a) (sizeof(a) / sizeof((a)[0]))

int native_call(VM* vm, int id) {
    NativeFn fn = NULL;

    if (id >= 0xD0) {
        if ((unsigned)(id - 0xD0) < COUNT(customTable))     fn = customTable[id - 0xD0];
    } else if (id >= 0xA0) {
        if ((unsigned)(id - 0xA0) < COUNT(capabilityTable)) fn = capabilityTable[id - 0xA0];
    } else {
        if ((unsigned)id < COUNT(baseTable))                fn = baseTable[id];
    }

    if (!fn) {
        char num[24];
        i64_to_str(id, num);
        send_uart("Unknown function index: ");
        send_uart(num);
        send_uart("\n");
        vm->halt = 1;
        return -1;
    }
    return fn(vm);
}
