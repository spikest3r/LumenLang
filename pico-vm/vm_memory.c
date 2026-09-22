/*
 * String heap (mark & sweep) and array storage.
 *
 * Every dynamically created string (JOIN, substr, int2str, input, ...) lives
 * in this heap as an immutable object. Variants carry heap=1 when they point
 * at one. The collector only runs BETWEEN instructions, so operands held in C
 * locals inside a single instruction are always safe.
 *
 * Roots: value stack, variables, array cells.
 * Strings from the program's string pool are static (heap=0) and never freed.
 */
#include "vm.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#define GC_MIN_SLACK  (16u * 1024u)
#define GC_MAX_SLACK  (64u * 1024u)

typedef struct StrObj {
    struct StrObj* next;
    uint32_t       size;      /* total malloc size, for accounting */
    uint8_t        marked;
    char           data[];
} StrObj;

typedef struct {
    Variant* cells;           /* NULL = array does not exist yet */
    uint32_t length;
    uint32_t capacity;
} Array;

static StrObj* g_objs = NULL;
static size_t  g_heapBytes = 0;
static size_t  g_gcThreshold = GC_MIN_SLACK;
static Array   g_arrays[MAX_ARRAYS];

/* ------------------------------------------------------------------ */
/* String heap                                                         */
/* ------------------------------------------------------------------ */
char* str_alloc(size_t len) {
    size_t total = sizeof(StrObj) + len + 1;
    StrObj* o = (StrObj*)malloc(total);
    if (!o) return NULL;
    o->next   = g_objs;
    o->size   = (uint32_t)total;
    o->marked = 0;
    o->data[len] = '\0';
    g_objs = o;
    g_heapBytes += total;
    return o->data;
}

size_t mem_heap_bytes(void) { return g_heapBytes; }
int    mem_gc_due(void)     { return g_heapBytes > g_gcThreshold; }

static void mark_variant(const Variant* v) {
    if (v->type == TAG_STRING && v->heap && v->data.str) {
        StrObj* o = (StrObj*)(v->data.str - offsetof(StrObj, data));
        o->marked = 1;
    }
}

void mem_gc(VM* vm) {
    /* mark */
    for (int i = 0; i <= vm->sp; i++)        mark_variant(&vm->stack[i]);
    for (int i = 0; i < MAX_VARIABLES; i++)  mark_variant(&vm->variables[i]);
    for (int a = 0; a < MAX_ARRAYS; a++) {
        const Array* arr = &g_arrays[a];
        for (uint32_t i = 0; i < arr->length; i++) mark_variant(&arr->cells[i]);
    }

    /* sweep */
    StrObj** link = &g_objs;
    while (*link) {
        StrObj* o = *link;
        if (o->marked) {
            o->marked = 0;
            link = &o->next;
        } else {
            *link = o->next;
            g_heapBytes -= o->size;
            free(o);
        }
    }

    /* next threshold: live + clamp(live, 16 KB, 64 KB) */
    size_t slack = g_heapBytes;
    if (slack < GC_MIN_SLACK) slack = GC_MIN_SLACK;
    if (slack > GC_MAX_SLACK) slack = GC_MAX_SLACK;
    g_gcThreshold = g_heapBytes + slack;
}

/* ------------------------------------------------------------------ */
/* Arrays  (ARRWRITE / ARRREAD)                                        */
/* ------------------------------------------------------------------ */
/* Created on first write, grown by doubling (min capacity 4), gaps are
 * filled with INT 0. Strings are immutable so cells just hold the Variant;
 * the GC keeps referenced strings alive. */
const char* array_write(int arrayIndex, int64_t index, const Variant* v) {
    if (index < 0)                  return "ARRWRITE: negative index";
    if (index >= MAX_ARRAY_LEN)     return "ARRWRITE: index too large for this platform";

    uint32_t needed = (uint32_t)index + 1;
    Array* a = &g_arrays[arrayIndex];

    if (!a->cells) {
        uint32_t cap = needed < 4 ? 4 : needed;
        a->cells = (Variant*)malloc(cap * sizeof(Variant));
        if (!a->cells) return "ARRWRITE: out of memory";
        a->length   = 0;
        a->capacity = cap;
    }

    if (needed > a->capacity) {
        uint32_t newCap = a->capacity;
        while (newCap < needed) newCap *= 2;
        Variant* nc = (Variant*)realloc(a->cells, newCap * sizeof(Variant));
        if (!nc) return "ARRWRITE: out of memory";
        a->cells    = nc;
        a->capacity = newCap;
    }

    for (uint32_t i = a->length; i < needed; i++) a->cells[i] = v_int(0);
    if (needed > a->length) a->length = needed;

    a->cells[index] = *v;
    return NULL;
}

const char* array_read(int arrayIndex, int64_t index, Variant* out) {
    const Array* a = &g_arrays[arrayIndex];
    if (!a->cells)                                    return "ARRREAD: array does not exist";
    if (index < 0 || (uint64_t)index >= a->length)    return "ARRREAD: index out of range";
    *out = a->cells[index];
    return NULL;
}
