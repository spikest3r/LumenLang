# Opcode Reference

Every instruction is a sequence of `int`s in the bytecode stream: one opcode int, followed by zero or more operand ints. "Size" below is the total instruction length (opcode + operands), i.e. what `getOpCodeOffset()` returns and how far `PC` advances by default.

## Control & data movement

| Opcode | Mnemonic | Size | Operands | Behavior |
|---|---|---|---|---|
| `0x01` | `CALL` | 2 | target offset (`uint8_t`) | Push `PC + 2` onto the return stack, jump to `target offset`. **Legacy 8-bit form** — the current compiler never emits this; see `CALL32` below |
| `0x02` | `POP` | 2 | variable slot | Pop the stack top into the given variable slot |
| `0x03` | `PUSH` | 3 | type tag, value | Push a value onto the stack — see below |
| `0x04` | `EXEC` | 2 | function index | Call a native function — see [Built-in Functions](./builtin-functions.md) |
| `0x05` | `JUMP` | 2 | target offset (`uint8_t`) | Unconditional jump. **Legacy 8-bit form** — the current compiler never emits this; see `JUMP32` below |
| `0x06` | `JUMP32` | 5 | target offset (`uint32_t`, little-endian) | Unconditional jump, 32-bit target. What `jump` actually compiles to |
| `0x07` | `CALL32` | 5 | target offset (`uint32_t`, little-endian) | Push `PC + 5` onto the return stack, jump to `target offset`. What `call` actually compiles to (used for `routine`/`call`) |
| `0xFE` | `RET` | 1 | — | Pop the return stack and jump there (used for `endroutine`) |
| `0xFF` | `HLT` | 1 | — | Halt execution |

### 8-bit vs. 32-bit addressing

Lumen's bytecode has two parallel families of jump/call/comparison opcodes: an original 8-bit-offset family (`CALL`/`JUMP`/`JEQ`.../`JNE`, single-byte target) and a newer 32-bit-offset family (`CALL32`/`JUMP32`/`JEQ32`.../`JNE32`, `uint32_t` little-endian target). **The current compiler only ever emits the 32-bit family** — this is what lets a program's bytecode exceed 255 bytes and still jump anywhere in it. The 8-bit opcodes are still recognized by the VM (`execute()` in `src/vm.cpp`) purely for backward compatibility with older, hand-written, or pre-32-bit-addressing bytecode; you won't see them in output from the current compiler. `BinaryProgram`'s container signature reflects this — see [Bytecode Format](../architecture/bytecode-format.md#the-bin-container) for the `0xFE 0xFD` (v4, "32-bit addressing") signature.

### `PUSH` type tags

The second operand of `PUSH` selects what the third operand means:

| Tag | Meaning |
|---|---|
| `0x01` | String — third operand is an index into the string pool |
| `0x02` | Integer — third operand is an index into the shared constant pool |
| `0x03` | Variable — third operand is a variable slot to read from |
| `0x04` | `uint8_t` literal — third operand is value itself |
| `0x05` | Float — third operand is an index into the shared constant pool, reinterpreted as `double` |

Integer and float constants share the same underlying pool (`constPool`, a `std::vector<double>`, deduplicated by `(type, value)` — see [Bytecode Format](../architecture/bytecode-format.md#the-const-pool)); tags `0x02` and `0x05` are both indices into it, distinguished only by which tag the `PUSH` instruction carries.

## Arithmetic

All arithmetic opcodes are size 1 (no operands) — they pop two `Variant` values off the stack, combine them, and push a `Variant` result. Operand order: the value pushed second (`b`) is popped first, so `a OP b` is computed correctly for non-commutative operators.

Arithmetic is type-aware: if **either** operand is `TAG_FLOAT`, both operands are read as `double` (`getNumeric()`) and the result is `TAG_FLOAT`. If both operands are `TAG_INT`, the result is `TAG_INT` — with one exception below.

| Opcode | Mnemonic | Operation | Result type |
|---|---|---|---|
| `0xA0` | `ADD` | `a + b` | Float if either operand is float, else int |
| `0xA1` | `SUB` | `a - b` | Float if either operand is float, else int |
| `0xA2` | `MUL` | `a * b` | Float if either operand is float, else int |
| `0xA3` | `DIV` | `a / b` | **Always float**, regardless of operand types |
| `0xA4` | `POW` | `a ^ b` (`std::pow`) | Float if either operand is float, else int (result truncated to `int64`) |
| `0xA5` | `MOD` | `a % b` (`std::fmod` if either is float, else integer `%`) | Float if either operand is float, else int |

`DIV` is the one exception to the "int stays int" rule: it always produces a `TAG_FLOAT` result even when both operands are integers, so `7 / 2` yields `3.5`, not `3`.

## Comparison / conditional jump

Both pop two values, read them as `double` via `getNumeric()` (so int and float operands compare correctly against each other), and either fall through (true) or jump to the operand (false). This is exactly how `if` blocks compile.

As with `CALL`/`JUMP` above, there are two parallel families here — an 8-bit legacy one and a 32-bit one the compiler actually generates:

| Opcode | Mnemonic | Size | Operand | Comparison |
|---|---|---|---|---|
| `0xB0` | `JEQ` | 2 | offset (`uint8_t`) | `==` — **legacy 8-bit form**, not emitted by the current compiler |
| `0xB1` | `JGR` | 2 | offset (`uint8_t`) | `>` — legacy |
| `0xB2` | `JLS` | 2 | offset (`uint8_t`) | `<` — legacy |
| `0xB3` | `JGE` | 2 | offset (`uint8_t`) | `>=` — legacy |
| `0xB4` | `JLE` | 2 | offset (`uint8_t`) | `<=` — legacy |
| `0xB5` | `JNE` | 2 | offset (`uint8_t`) | `!=` — legacy |
| `0xC0` | `JEQ32` | 5 | offset (`uint32_t`, little-endian) | `==` — what `if x == y` actually compiles to |
| `0xC1` | `JGR32` | 5 | offset (`uint32_t`, little-endian) | `>` |
| `0xC2` | `JLS32` | 5 | offset (`uint32_t`, little-endian) | `<` |
| `0xC3` | `JGE32` | 5 | offset (`uint32_t`, little-endian) | `>=` |
| `0xC4` | `JLE32` | 5 | offset (`uint32_t`, little-endian) | `<=` |
| `0xC5` | `JNE32` | 5 | offset (`uint32_t`, little-endian) | `!=` |

In both families, the operand is the offset to jump to **if the comparison is false** (i.e. it's the "skip the true-branch body" target, not a "jump if true" target).

## Strings

| Opcode | Mnemonic | Size | Behavior |
|---|---|---|---|
| `0xAA` | `JOIN` | 1 | Pop a count `n`, then pop `n` strings off the stack and concatenate them in original order, push the result — this is what `..` compiles to |

## Notes

- Arithmetic and comparison opcodes are shared between `TAG_INT` and `TAG_FLOAT` operands — there's no separate float-only opcode set. Type promotion happens per-operation, as described above (see [Bytecode Format](../architecture/bytecode-format.md#values-on-the-stack-variant) for the `Variant` type).
- The 8-bit (`CALL`, `JUMP`, `JEQ`...`JNE`) and 32-bit (`CALL32`, `JUMP32`, `JEQ32`...`JNE32`) opcode families are functionally identical apart from operand width — the compiler always chooses 32-bit so program size isn't limited to 255 bytes. `--disassemble` output from the current toolchain will only ever show the `...32` mnemonics; the 8-bit forms exist in the VM for backward compatibility with older bytecode.
- Unrecognized opcodes cause the VM to print `Invalid opcode` and halt with an error.
