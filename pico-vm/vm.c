/*
 * Lumen VM core: instruction dispatch.
 * Mirrors the desktop VM (vm.cpp) opcode-for-opcode.
 */
#include "vm.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */
int vm_error(VM* vm, const char* prefix, const char* msg) {
    send_uart(prefix);
    send_uart("\n");
    send_uart(msg);
    send_uart("\n");
    vm->halt = 1;
    return -1;
}

static int vm_fail(VM* vm, const char* msg) {
    return vm_error(vm, "Runtime error", msg);
}

int vm_push(VM* vm, Variant v) {
    if (vm->sp >= MAX_STACK - 1) return vm_fail(vm, "stack overflow");
    vm->stack[++vm->sp] = v;
    return 0;
}

int vm_pop(VM* vm, Variant* out) {
    if (vm->sp < 0) return vm_fail(vm, "stack underflow");
    *out = vm->stack[vm->sp--];
    return 0;
}

size_t i64_to_str(int64_t v, char* buf) {
    char tmp[24];
    int n = 0;
    uint64_t u = (v < 0) ? (0 - (uint64_t)v) : (uint64_t)v;
    do { tmp[n++] = (char)('0' + (u % 10)); u /= 10; } while (u);
    char* o = buf;
    if (v < 0) *o++ = '-';
    while (n) *o++ = tmp[--n];
    *o = '\0';
    return (size_t)(o - buf);
}

static double getNumeric(const Variant* v) {
    if (v->type == TAG_FLOAT) return v->data.f;
    if (v->type == TAG_INT)   return (double)v->data.i;
    return 0.0;
}

/* Text form used by JOIN: strings as-is, ints as decimal, floats as "%f". */
static const char* variant_text(const Variant* v, char* tmp, size_t tmpLen) {
    switch (v->type) {
    case TAG_STRING: return v->data.str ? v->data.str : "";
    case TAG_INT:    i64_to_str(v->data.i, tmp); return tmp;
    case TAG_FLOAT:  snprintf(tmp, tmpLen, "%f", v->data.f); return tmp;
    }
    tmp[0] = '\0';
    return tmp;
}

static uint32_t readU32(const uint8_t* bc, int pos) {
    return (uint32_t)bc[pos]           |
           (uint32_t)bc[pos + 1] << 8  |
           (uint32_t)bc[pos + 2] << 16 |
           (uint32_t)bc[pos + 3] << 24;
}

static int getOpCodeOffset(int opcode) {
    switch (opcode) {
    case 0x03:
        return 3;
    case 0x01: case 0x02: case 0x04: case 0x05:
    case 0xA8: case 0xA9: case 0xAB:
    case 0xB0: case 0xB1: case 0xB2: case 0xB3: case 0xB4: case 0xB5:
    case 0xDA: case 0xDB:
        return 2;
    case 0x06: case 0x07:
    case 0xC0: case 0xC1: case 0xC2: case 0xC3: case 0xC4: case 0xC5:
        return 5;
    }
    return 1;
}

/* ------------------------------------------------------------------ */
/* Arithmetic  (0xA0 ADD, A1 SUB, A2 MUL, A3 DIV, A4 POW, A5 MOD)     */
/* ------------------------------------------------------------------ */
static int op_arith(VM* vm, int opcode) {
    /* desktop quirk: with fewer than 2 values it pushes INT 0 and touches nothing */
    if (vm->sp < 1) return vm_push(vm, v_int(0));

    Variant b = vm->stack[vm->sp];
    Variant a = vm->stack[vm->sp - 1];
    if (a.type == TAG_STRING || b.type == TAG_STRING)
        return vm_fail(vm, "type error: arithmetic on a string value");
    vm->sp -= 2;

    Variant r = v_int(0);

    if (opcode == 0xA3) {                               /* DIV: always float */
        r = v_float(getNumeric(&a) / getNumeric(&b));
    } else if (a.type == TAG_FLOAT || b.type == TAG_FLOAT) {
        double x = getNumeric(&a), y = getNumeric(&b);
        switch (opcode) {
        case 0xA0: r = v_float(x + y);      break;
        case 0xA1: r = v_float(x - y);      break;
        case 0xA2: r = v_float(x * y);      break;
        case 0xA4: r = v_float(pow(x, y));  break;
        case 0xA5: r = v_float(fmod(x, y)); break;
        }
    } else {
        int64_t x = a.data.i, y = b.data.i;
        switch (opcode) {
        case 0xA0: r = v_int((int64_t)((uint64_t)x + (uint64_t)y)); break;
        case 0xA1: r = v_int((int64_t)((uint64_t)x - (uint64_t)y)); break;
        case 0xA2: r = v_int((int64_t)((uint64_t)x * (uint64_t)y)); break;
        case 0xA4: r = v_int(dbl_to_i64(pow((double)x, (double)y))); break;
        case 0xA5:
            if (y == 0) return vm_fail(vm, "modulo by zero");
            r = v_int(y == -1 ? 0 : x % y);
            break;
        }
    }
    return vm_push(vm, r);
}

/* ------------------------------------------------------------------ */
/* Comparison (op = opcode & 0x0F: 0 ==, 1 >, 2 <, 3 >=, 4 <=, 5 !=)   */
/* ------------------------------------------------------------------ */
static int cmp_result(int op, const Variant* a, const Variant* b) {
    if (a->type == TAG_STRING && b->type == TAG_STRING) {
        int c = strcmp(a->data.str, b->data.str);
        switch (op) {
        case 0: return c == 0;
        case 1: return c >  0;
        case 2: return c <  0;
        case 3: return c >= 0;
        case 4: return c <= 0;
        default: return c != 0;
        }
    }
    double av = getNumeric(a), bv = getNumeric(b);
    switch (op) {
    case 0: return av == bv;
    case 1: return av >  bv;
    case 2: return av <  bv;
    case 3: return av >= bv;
    case 4: return av <= bv;
    default: return av != bv;
    }
}

/* ------------------------------------------------------------------ */
/* JOIN (0xAA): stack [s1 .. sN, N(top)] -> [concatenated string]      */
/* ------------------------------------------------------------------ */
static int op_join(VM* vm) {
    Variant cnt;
    if (vm_pop(vm, &cnt) < 0) return -1;
    if (cnt.type != TAG_INT) return vm_fail(vm, "JOIN: count must be an integer");
    if (cnt.data.i < 0 || cnt.data.i > vm->sp + 1) return vm_fail(vm, "JOIN: stack underflow");

    int n = (int)cnt.data.i;
    const Variant* items = &vm->stack[vm->sp + 1 - n];
    char tmp[64];

    size_t total = 0;
    for (int i = 0; i < n; i++)
        total += strlen(variant_text(&items[i], tmp, sizeof(tmp)));

    char* out = str_alloc(total);
    if (!out) return vm_fail(vm, "out of memory");

    size_t pos = 0;
    for (int i = 0; i < n; i++) {
        const char* t = variant_text(&items[i], tmp, sizeof(tmp));
        size_t l = strlen(t);
        memcpy(out + pos, t, l);
        pos += l;
    }

    vm->sp -= n;
    return vm_push(vm, v_str_heap(out));
}

/* ------------------------------------------------------------------ */
/* CALL / RET                                                          */
/* ------------------------------------------------------------------ */
static int do_call(VM* vm, int returnPC, int target) {
    if (vm->csp >= MAX_CALL_DEPTH - 1) return vm_fail(vm, "call stack overflow");
    vm->csp++;
    vm->callPC[vm->csp]   = returnPC;
    vm->callBase[vm->csp] = vm->routineBase;
    vm->routineBase = target;
    return 0;
}

/* ------------------------------------------------------------------ */
/* Single instruction. Returns next PC, or -1 to stop.                 */
/* ------------------------------------------------------------------ */
static int vm_step(VM* vm) {
    const uint8_t* bc = vm->prog->bytecode;
    int PC = vm->PC;

    if (PC < 0 || PC >= vm->prog->bytecodeSize)
        return vm_fail(vm, "program counter out of range");

    int opcode = bc[PC];
    int offset = getOpCodeOffset(opcode);
    if (PC + offset > vm->prog->bytecodeSize)
        return vm_fail(vm, "truncated instruction");

    switch (opcode) {

    /* ---- control flow ---- */
    case 0x01: {                                        /* CALL (8-bit) */
        int addr = bc[PC + 1];
        if (do_call(vm, PC + offset, addr) < 0) return -1;
        return addr;
    }
    case 0xFE: {                                        /* RET */
        if (vm->csp < 0) {
            send_uart("Return stack underflow!\n");
            vm->halt = 1;
            return -1;
        }
        int ret = vm->callPC[vm->csp];
        vm->routineBase = vm->callBase[vm->csp];
        vm->csp--;
        return ret;
    }
    case 0x05:                                          /* JUMP (8-bit) */
        return vm->routineBase + bc[PC + 1];
    case 0x06:                                          /* JUMP32 */
        return vm->routineBase + (int)readU32(bc, PC + 1);
    case 0x07: {                                        /* CALL32 */
        int target = (int)readU32(bc, PC + 1);
        if (do_call(vm, PC + offset, target) < 0) return -1;
        return target;
    }

    /* ---- stack / variables ---- */
    case 0x02: {                                        /* STORE var */
        Variant v;
        if (vm_pop(vm, &v) < 0) return -1;
        vm->variables[bc[PC + 1]] = v;
        break;
    }
    case 0x03: {                                        /* PUSH type,value */
        int dataType = bc[PC + 1];
        int value    = bc[PC + 2];
        switch (dataType) {
        case 0x01:
            if (value >= vm->prog->stringCount) return vm_fail(vm, "string pool index out of range");
            if (vm_push(vm, v_str_static(vm->prog->strings[value])) < 0) return -1;
            break;
        case 0x02:
            if (value >= vm->prog->constCount) return vm_fail(vm, "const pool index out of range");
            if (vm_push(vm, v_int(dbl_to_i64(vm->prog->consts[value]))) < 0) return -1;
            break;
        case 0x03: {
            const Variant* var = &vm->variables[value];
            /* never-written variable reads as INT 0 */
            if (vm_push(vm, var->type == TAG_NONE ? v_int(0) : *var) < 0) return -1;
            break;
        }
        case 0x04:
            if (vm_push(vm, v_int(value)) < 0) return -1;
            break;
        case 0x05:
            if (value >= vm->prog->constCount) return vm_fail(vm, "const pool index out of range");
            if (vm_push(vm, v_float(vm->prog->consts[value])) < 0) return -1;
            break;
        }
        break;
    }
    case 0xAB:                                          /* CPY var (no pop) */
        if (vm->sp < 0) return vm_fail(vm, "stack underflow");
        vm->variables[bc[PC + 1]] = vm->stack[vm->sp];
        break;
    case 0xDE: {                                        /* DEREF */
        Variant p;
        if (vm_pop(vm, &p) < 0) return -1;
        if (p.type != TAG_INT || p.data.i < 0 || p.data.i >= MAX_VARIABLES ||
            vm->variables[p.data.i].type == TAG_NONE) {
            send_uart("Invalid dereference\n");
            vm->halt = 1;
            return -1;
        }
        if (vm_push(vm, vm->variables[p.data.i]) < 0) return -1;
        break;
    }

    /* ---- native call ---- */
    case 0x04:
        if (native_call(vm, bc[PC + 1]) < 0) return -1;
        break;

    /* ---- arithmetic ---- */
    case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4: case 0xA5:
        if (op_arith(vm, opcode) < 0) return -1;
        break;

    /* ---- INC / DEC (top of stack) ---- */
    case 0xA6: case 0xA7:
        if (vm->sp >= 0) {
            Variant* x = &vm->stack[vm->sp];
            int d = (opcode == 0xA6) ? 1 : -1;
            if (x->type == TAG_INT)        x->data.i = (int64_t)((uint64_t)x->data.i + (uint64_t)(int64_t)d);
            else if (x->type == TAG_FLOAT) x->data.f += d;
        }
        break;

    /* ---- INCV / DECV (variable) ---- */
    case 0xA8: case 0xA9: {
        Variant* x = &vm->variables[bc[PC + 1]];
        int d = (opcode == 0xA8) ? 1 : -1;
        if (x->type == TAG_INT)        x->data.i = (int64_t)((uint64_t)x->data.i + (uint64_t)(int64_t)d);
        else if (x->type == TAG_FLOAT) x->data.f += d;
        break;
    }

    /* ---- compare + conditional jump ---- */
    case 0xB0: case 0xB1: case 0xB2: case 0xB3: case 0xB4: case 0xB5: {
        Variant a, b;
        if (vm->sp < 1) {                               /* desktop quirk: sentinel operands */
            a = v_int(0xFEEDFACE);
            b = v_int(0xDEADBEEF);
        } else {
            b = vm->stack[vm->sp--];
            a = vm->stack[vm->sp--];
        }
        if (!cmp_result(opcode & 0x0F, &a, &b))
            return vm->routineBase + bc[PC + 1];
        break;
    }
    case 0xC0: case 0xC1: case 0xC2: case 0xC3: case 0xC4: case 0xC5: {
        if (vm->sp < 1) return vm_fail(vm, "stack underflow");
        Variant b = vm->stack[vm->sp--];
        Variant a = vm->stack[vm->sp--];
        if (!cmp_result(opcode & 0x0F, &a, &b))
            return vm->routineBase + (int)readU32(bc, PC + 1);
        break;
    }

    /* ---- strings ---- */
    case 0xAA:
        if (op_join(vm) < 0) return -1;
        break;

    /* ---- arrays ---- */
    case 0xDA: {                                        /* ARRWRITE arr: [value, index(top)] -> [] */
        int arr = bc[PC + 1];
        Variant idx, val;
        if (vm_pop(vm, &idx) < 0 || vm_pop(vm, &val) < 0) return -1;
        if (idx.type != TAG_INT) return vm_fail(vm, "ARRWRITE: index must be an integer");
        const char* err = array_write(arr, idx.data.i, &val);
        if (err) return vm_fail(vm, err);
        break;
    }
    case 0xDB: {                                        /* ARRREAD arr: [index(top)] -> [value] */
        int arr = bc[PC + 1];
        Variant idx, val;
        if (vm_pop(vm, &idx) < 0) return -1;
        if (idx.type != TAG_INT) return vm_fail(vm, "ARRREAD: index must be an integer");
        const char* err = array_read(arr, idx.data.i, &val);
        if (err) return vm_fail(vm, err);
        if (vm_push(vm, val) < 0) return -1;
        break;
    }

    case 0xFF:                                          /* HALT */
        vm->halt = 1;
        break;

    default: {
        char num[24];
        send_uart("Invalid opcode: ");
        i64_to_str(opcode, num);
        send_uart(num);
        send_uart("\n");
        vm->halt = 1;
        return -1;
    }
    }

    return PC + offset;
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */
void vm_init(VM* vm, const LumenProgram* prog) {
    memset(vm, 0, sizeof(*vm));
    vm->prog = prog;
    vm->sp   = -1;
    vm->csp  = -1;
    vm->PC   = 0;
    vm->routineBase = 0;
}

int vm_run(VM* vm) {
    while (1) {
        int result = vm_step(vm);
        if (vm->halt || result < 0) break;
        vm->PC = result;

        /* GC only between instructions, when VM state is fully consistent */
        if (mem_gc_due()) mem_gc(vm);
    }
    return 0;
}
