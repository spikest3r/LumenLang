#ifndef LUMEN_VM_H
#define LUMEN_VM_H

#include <stdint.h>
#include <stddef.h>

/* ------------------------------------------------------------------ */
/* Limits (tuned for RP2040: 264 KB RAM, 2 KB default C stack)         */
/* ------------------------------------------------------------------ */
#define MAX_STACK        64      /* value stack depth                   */
#define MAX_VARIABLES    256     /* variable index is a uint8           */
#define MAX_CALL_DEPTH   64      /* CALL/CALL32 nesting                 */
#define MAX_CONST_POOL   256     /* const index is a uint8              */
#define MAX_STRING_POOL  256     /* string index is a uint8             */
#define MAX_ARRAYS       256     /* array index is a uint8              */
#define MAX_ARRAY_LEN    16384   /* elements per array                  */
#define INPUT_BUF_LEN    128

/* ------------------------------------------------------------------ */
/* Values                                                              */
/* ------------------------------------------------------------------ */
enum { TAG_NONE = 0, TAG_STRING = 1, TAG_INT = 2, TAG_FLOAT = 3 };
typedef uint8_t TypeTag;

typedef struct {
    TypeTag type;
    uint8_t heap;               /* 1 = str points into the GC string heap */
    union {
        int64_t i;
        double  f;
        char*   str;
    } data;
} Variant;

static inline Variant v_int(int64_t i) {
    Variant v; v.type = TAG_INT; v.heap = 0; v.data.i = i; return v;
}
static inline Variant v_float(double f) {
    Variant v; v.type = TAG_FLOAT; v.heap = 0; v.data.f = f; return v;
}
static inline Variant v_str_static(char* s) {   /* string pool / literals */
    Variant v; v.type = TAG_STRING; v.heap = 0; v.data.str = s; return v;
}
static inline Variant v_str_heap(char* s) {     /* from str_alloc()       */
    Variant v; v.type = TAG_STRING; v.heap = 1; v.data.str = s; return v;
}

static inline int64_t dbl_to_i64(double d) {
    if (d != d) return 0;
    if (d >= 9223372036854775807.0)  return INT64_MAX;
    if (d <= -9223372036854775808.0) return INT64_MIN;
    return (int64_t)d;
}

/* ------------------------------------------------------------------ */
/* Loaded program                                                      */
/* ------------------------------------------------------------------ */
typedef struct {
    const uint8_t* bytecode;    /* points straight into flash (no copy) */
    int            bytecodeSize;
    char**         strings;
    int            stringCount;
    double         consts[MAX_CONST_POOL];
    int            constCount;
    int            variableCount;
} LumenProgram;

/* ------------------------------------------------------------------ */
/* VM state                                                            */
/* ------------------------------------------------------------------ */
typedef struct {
    const LumenProgram* prog;
    Variant stack[MAX_STACK];
    int     sp;                         /* -1 = empty */
    Variant variables[MAX_VARIABLES];
    int     callPC[MAX_CALL_DEPTH];
    int     callBase[MAX_CALL_DEPTH];
    int     csp;                        /* -1 = empty */
    int     PC;
    int     routineBase;
    int     halt;
} VM;

/* vm.c */
void   vm_init(VM* vm, const LumenProgram* prog);
int    vm_run(VM* vm);
int    vm_error(VM* vm, const char* prefix, const char* msg); /* prints, halts, returns -1 */
int    vm_push(VM* vm, Variant v);                            /* 0 ok, -1 error */
int    vm_pop(VM* vm, Variant* out);                          /* 0 ok, -1 error */
size_t i64_to_str(int64_t v, char* buf);                      /* buf >= 21 bytes */

/* natives.c */
int native_call(VM* vm, int id);

/* vm_memory.c : GC'd string heap + arrays */
char*       str_alloc(size_t len);              /* NUL-terminated, len+1 bytes; NULL on OOM */
int         mem_gc_due(void);
void        mem_gc(VM* vm);
size_t      mem_heap_bytes(void);
const char* array_write(int arrayIndex, int64_t index, const Variant* v); /* NULL or error */
const char* array_read(int arrayIndex, int64_t index, Variant* out);      /* NULL or error */

/* loader.c */
int loadFromFlash(LumenProgram* p, const uint8_t* data, int dataSize);

/* main.c (platform I/O) */
void send_uart(const char* message);
void uart_readline(char* buffer, int maxLen);

#endif
