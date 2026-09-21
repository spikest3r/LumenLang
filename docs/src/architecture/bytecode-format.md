# Bytecode Format

## The .bin container

All integers are little-endian.

| Field | Size | Notes |
|---|---|---|
| Signature | 2 bytes | `FE FF` for the current format. |
| Bytecode size | int32 | |
| Bytecode | n bytes | |
| String pool size | int32 | number of strings |
| String entries | repeated | int32 length + bytes |
| Constant pool size | int32 | number of constants |
| Constants | repeated | 8-byte doubles |
| Variable count | int32 | |
| Debug data size | int32 | `0` with `--no-debug` |
| Debug data | n bytes | text, see [Debug Symbols](../debugging/debug-symbols.md) |

Signatures the loader understands: `FE FB` (v2), `FE FC` (v2.1, double constants), `FE FD` (v2.2, 32-bit addressing), `FE FE` (v3) and `FE FF` (v4, current). `FE FA` (v1) is rejected. Older files load without debug data, because the trailing debug section is optional.

## Values on the stack (Variant)

The VM stack holds tagged values: integer (`int64`), float (`double`) or string. An internal array tag exists for the memory manager but arrays are not first-class values.

## Instruction encoding

Instructions are a one-byte opcode followed by byte operands:

- Variable, string, constant and array operands are **one byte** (an index into its pool), which is why a program can use at most 256 of each.
- Jump and call targets are **four bytes** (`uint32`). Legacy 8-bit jump forms remain in the opcode table.
- Jump targets inside a routine are relative to the start of that routine; the VM adds the routine's base address.

## PUSH type tags

`PUSH` is three bytes: opcode, a tag, and an operand.

| Tag | Operand |
|---|---|
| `0x01` | index into the string pool |
| `0x02` | index into the integer constant pool |
| `0x03` | variable slot to read |
| `0x04` | an immediate integer (used for references `&x` and the operand count of `JOIN`) |
| `0x05` | index into the float constant pool |

## The const pool

Numbers that appear in source are stored once, in the constant pool, and pushed by index. Floats are stored as doubles.

## Program layout

```text
main program ... HLT
routine 0 body ... RET
routine 1 body ... RET
```

Calls to routines are `CALL32 <absolute address>`. Imported code is part of the main program.

## Limits

256 variables, 256 arrays, 256 string literals and 256 numeric constants per program (one-byte operands, not checked by the compiler).
