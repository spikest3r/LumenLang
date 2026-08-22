# Bytecode Format

## Instruction encoding

Lumen's bytecode is not byte-packed — it's a flat `std::vector<uint8_t>`. Each instruction is one or more `uint8_t`s: an opcode, followed by however many operand ints that opcode needs. There's no length prefix per instruction; the VM knows how many operands to consume from the opcode alone, via `getOpCodeOffset()`.

For the full opcode table, see [Opcode Reference](../reference/opcodes.md).

## The `.bin` container

Compiled programs are written to disk with `BinaryProgram::save()` (`src/programfile.cpp`) in a small custom binary format:

| Field | Type | Description |
|---|---|---|
| Signature | 2 bytes | `0xFE 0xFA` (v1) / `0xFE 0xFB` (v2) / `0xFE 0xFC` (v3) / `0xFE 0xFD` (v4, current) — validated on load (see below), load fails if it doesn't match a known signature |
| Bytecode length | `int32` | Number of ints in the bytecode stream |
| Bytecode | `uint8_t[length]` | The instruction stream itself |
| String pool size | `int32` | Number of pooled string literals |
| String pool entries | repeated `{int32 len, char[len]}` | Each string literal, length-prefixed, **not** null-terminated |
| Const pool size | `int32` | Number of pooled constants |
| Const pool entries | `double[length]` (v3) / `int32[length]` (v2 and earlier) | Shared integer + float constant array — see below |
| Variable count | `int32` | Number of variable slots to allocate at VM startup |

This is the entire file — no header versioning beyond the 2-byte signature, no section table. It's deliberately minimal.

### Signature versions and backward compatibility

`BinaryProgram::load()` (`src/programfile.cpp`) checks the signature's second byte to decide how to read the const pool:

- **`0xFE 0xFA` (v1)** — refused outright; `load()` prints a message that v1 binaries aren't compatible with the current runtime and fails.
- **`0xFE 0xFB` (v2)** — const pool entries are read as `int32`, then widened to `double` on load, so old integer-only bytecode still runs correctly on the current VM.
- **`0xFE 0xFC` (v3)** — const pool entries are read directly as `double`.
- **`0xFE 0xFD` (v4, "32-bit addressing")** — const pool is read identically to v3 (directly as `double`). The version bump reflects a *bytecode* change, not a container change: the compiler now emits the 32-bit-offset jump/call opcodes (`JUMP32`, `CALL32`, `JEQ32`...`JNE32`) instead of their 8-bit predecessors, so program size is no longer capped at 255 bytes for jump targets — see [Opcode Reference](../reference/opcodes.md#8-bit-vs-32-bit-addressing). This is the format the current compiler writes.

Everything else in the file (bytecode, string pool, variable count) is unchanged across versions — only the const pool's on-disk element type (v2 vs. v3/v4) and the bytecode's addressing width (pre-v4 vs. v4) differ.

## The string pool

Every string literal in the source is deduplicated into a single pool during compilation (`resolveString()` in `src/helpers.cpp`); a `PUSH` instruction for a string operand carries an index into this pool rather than embedding the text inline in the bytecode stream.

## The const pool

Integer and float literals share a single deduplicated pool (`resolveConst()` in `src/helpers.cpp`), keyed on `(TypeTag, value)` so an int `2` and a float `2.0` get distinct entries even though they'd compare equal as raw numbers. A `PUSH` instruction carries the pool index plus a type tag (`0x02` for int, `0x05` for float — see [Opcode Reference](../reference/opcodes.md#push-type-tags)) that tells the VM how to reinterpret the stored `double` when it lands on the stack.

## Variables are slots, not names

By the time a program reaches the `.bin` file, variable names are gone — the compiler maps each name to an integer slot index (`resolveVariableIndex()`), and the VM allocates a flat `std::vector<Variant>` of that size at startup. This is why a `.bin` file alone is unreadable to a human: `--disassemble` will show you `variable index 3`, not the original name. To get names back, you need the separate debug symbols file — see [Debug Symbols](../debugging/debug-symbols.md).

## Values on the stack: `Variant`

Every value that moves through the VM — on the stack or in a variable slot — is a tagged union (`include/types.h`):

```cpp
typedef enum {
    TAG_INT = 2,
    TAG_FLOAT = 3,
    TAG_STRING = 1
} TypeTag;

typedef struct {
    TypeTag type;
    std::variant<int64_t, double, std::string> data;
} Variant;
```

Arithmetic and comparison opcodes read both `TAG_INT` and `TAG_FLOAT` operands, promoting to `double` whenever either side is a float (see [Opcode Reference](../reference/opcodes.md#arithmetic) for the exact per-opcode behavior, including `DIV`'s always-float result).
