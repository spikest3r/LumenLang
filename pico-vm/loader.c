/*
 * Program loader.
 *
 * Layout (little endian):
 *   [sig0 sig1] [bcSize:i32] [bytecode...]
 *   [stringCount:i32] { [len:i32] [bytes...] } *
 *   [constCount:i32]  { double (v3+) | int32 (v2) } *
 *   [variableCount:i32]
 *
 * Signatures: FE FB (v2, int32 consts), FE FC (v3, double consts),
 *             FE FD (v4, 32-bit jumps), FE FE (v5). v1 (FE FA) is rejected.
 */
#include "vm.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    const uint8_t* data;
    int size;
    int pos;
    int ok;
} Reader;

static uint8_t rd_u8(Reader* r) {
    if (r->pos + 1 > r->size) { r->ok = 0; return 0; }
    return r->data[r->pos++];
}

static int32_t rd_i32(Reader* r) {
    if (r->pos + 4 > r->size) { r->ok = 0; return 0; }
    const uint8_t* d = r->data + r->pos;
    r->pos += 4;
    return (int32_t)((uint32_t)d[0] | (uint32_t)d[1] << 8 | (uint32_t)d[2] << 16 | (uint32_t)d[3] << 24);
}

static double rd_double(Reader* r) {
    double v = 0.0;
    if (r->pos + (int)sizeof(double) > r->size) { r->ok = 0; return 0.0; }
    memcpy(&v, r->data + r->pos, sizeof(double));
    r->pos += (int)sizeof(double);
    return v;
}

static int fail(const char* msg) {
    send_uart(msg);
    return -1;
}

int loadFromFlash(LumenProgram* p, const uint8_t* data, int dataSize) {
    Reader r = { data, dataSize, 0, 1 };
    memset(p, 0, sizeof(*p));

    uint8_t sig0 = rd_u8(&r);
    uint8_t sig1 = rd_u8(&r);
    int isV2 = (sig0 == 0xFE && sig1 == 0xFB);
    int isV3 = (sig0 == 0xFE && sig1 == 0xFC);
    int isV4 = (sig0 == 0xFE && sig1 == 0xFD);
    int isV5 = (sig0 == 0xFE && sig1 == 0xFE);

    if (!r.ok || (!isV2 && !isV3 && !isV4 && !isV5)) {
        send_uart("Invalid signature\n");
        if (sig0 == 0xFE && sig1 == 0xFA)
            send_uart("v1 Precompiled Lumen binaries are not compatible with v2+ Lumen runtime\n");
        return -1;
    }

    /* bytecode: referenced in place (flash), not copied to RAM */
    int bcSize = rd_i32(&r);
    if (!r.ok || bcSize <= 0 || bcSize > r.size - r.pos) return fail("Invalid bytecode size\n");
    p->bytecode     = data + r.pos;
    p->bytecodeSize = bcSize;
    r.pos += bcSize;

    /* string pool */
    int spSize = rd_i32(&r);
    if (!r.ok || spSize < 0 || spSize > MAX_STRING_POOL) return fail("Invalid string pool size\n");
    if (spSize > 0) {
        p->strings = (char**)calloc((size_t)spSize, sizeof(char*));
        if (!p->strings) return fail("malloc failed\n");
    }
    for (int i = 0; i < spSize; i++) {
        int len = rd_i32(&r);
        if (!r.ok || len < 0 || len > r.size - r.pos) return fail("Invalid string pool entry\n");
        char* s = (char*)malloc((size_t)len + 1);
        if (!s) return fail("malloc failed\n");
        memcpy(s, data + r.pos, (size_t)len);
        s[len] = '\0';
        r.pos += len;
        p->strings[i] = s;
        p->stringCount = i + 1;
    }

    /* const pool */
    int cpSize = rd_i32(&r);
    if (!r.ok || cpSize < 0 || cpSize > MAX_CONST_POOL) return fail("Invalid const pool size\n");
    for (int i = 0; i < cpSize; i++)
        p->consts[i] = isV2 ? (double)rd_i32(&r) : rd_double(&r);
    p->constCount = cpSize;

    /* variable count */
    p->variableCount = rd_i32(&r);
    if (!r.ok) return fail("Program data truncated\n");
    if (p->variableCount < 0 || p->variableCount > MAX_VARIABLES) return fail("Too many variables\n");

    return 0;
}
