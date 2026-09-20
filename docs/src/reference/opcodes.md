# Opcode Reference

Sizes are in bytes, opcode included. `S` is the operand stack, top on the right.

## Stack and variables

| Opcode | Mnemonic | Size | Effect |
|---|---|---|---|
| `0x02` | `POP v` | 2 | Pop a value into variable slot `v`. |
| `0x03` | `PUSH tag x` | 3 | Push a value, see [PUSH type tags](#push-type-tags). |
| `0xA6` | `INC` | 1 | Add 1 to the top of the stack. |
| `0xA7` | `DEC` | 1 | Subtract 1 from the top of the stack. |
| `0xA8` | `INCV v` | 2 | Increment variable slot `v`. |
| `0xA9` | `DECV v` | 2 | Decrement variable slot `v`. |
| `0xAB` | `CPY v` | 2 | Copy the top of the stack into variable slot `v` **without popping it** (used for the `repeat` iterator). |
| `0xDE` | `DEREF` | 1 | Pop a slot number and push that variable's value (`*r`). Stops the VM if the slot does not exist. |

### PUSH type tags

| Tag | Operand |
|---|---|
| `0x01` | string pool index |
| `0x02` | integer constant pool index |
| `0x03` | variable slot to read |
| `0x04` | immediate integer (references, `JOIN` count) |
| `0x05` | float constant pool index |

## Arithmetic

All pop two values and push one. Integer operands give an integer; a float operand gives a float. A string operand is a runtime error.

| Opcode | Mnemonic | Effect |
|---|---|---|
| `0xA0` | `ADD` | `a + b` |
| `0xA1` | `SUB` | `a - b` |
| `0xA2` | `MUL` | `a * b` |
| `0xA3` | `DIV` | `a / b` (always floating point) |
| `0xA4` | `POW` | `a ^ b` |
| `0xA5` | `MOD` | `a % b` (integer `% 0` is a runtime error) |
| `0xAA` | `JOIN` | Pop a count `n`, then join the next `n` values into one string (`..`). |

## Arrays

| Opcode | Mnemonic | Size | Effect |
|---|---|---|---|
| `0xDA` | `ARRWRITE a` | 2 | Stack `[value, index]`: store `value` in array `a`, creating or growing it. |
| `0xDB` | `ARRREAD a` | 2 | Stack `[index]`: push element `index` of array `a`. |

Errors: negative write index, missing array, index out of range.

## Control flow

### 8-bit vs 32-bit addressing

The current compiler emits only the 32-bit forms (opcode + 4-byte address, 5 bytes). The 8-bit forms (2 bytes) are kept so older programs still run.

| 8-bit | 32-bit | Mnemonic | Effect |
|---|---|---|---|
| `0x05` | `0x06` | `JUMP` | Unconditional jump. |
| `0x01` | `0x07` | `CALL` | Push a frame and jump to a routine. |
| `0xB0` | `0xC0` | `JEQ` | Pop `b`, `a`; jump if `a == b`. |
| `0xB1` | `0xC1` | `JGR` | jump if `a > b` |
| `0xB2` | `0xC2` | `JLS` | jump if `a < b` |
| `0xB3` | `0xC3` | `JGE` | jump if `a >= b` |
| `0xB4` | `0xC4` | `JLE` | jump if `a <= b` |
| `0xB5` | `0xC5` | `JNE` | jump if `a != b` |

Two strings compare lexicographically, other values numerically. Jump targets inside a routine are relative to the routine's start.

| Opcode | Mnemonic | Size | Effect |
|---|---|---|---|
| `0x04` | `EXEC f` | 2 | Call built-in function `f`, see [Built-in Functions](./builtin-functions.md). |
| `0xFE` | `RET` | 1 | Return from a routine. |
| `0xFF` | `HLT` | 1 | Stop the program. |
