# Compiler Pipeline

Lumen follows the classic compiler pipeline: source text goes in, bytecode comes out, a virtual machine executes it.

```
Source Code (.lmn)
        |
        v
   Lumen Compiler
        |
        v
 Bytecode (.bin)
        |
        v
 Virtual Machine
        |
        v
     Output
```

## Compilation

`compile()` (`src/compiler.cpp`) reads the source file **line by line**. Each line is tokenized by `tokenizeFormula()` (`src/tokenizer.cpp`), which splits on whitespace and operators while respecting single-quoted string boundaries, and drops anything after a `#`.

Each line's leading token decides what gets emitted:

- A bare identifier followed by `=` becomes an **assignment**: the right-hand side is evaluated and pushed onto the value stack, and an assignment instruction pops it into the target variable's slot.
- `println` / `print` / `inputInt` / `inputStr` become **calls** into the VM's native function table (see [Built-in Functions](../reference/builtin-functions.md)).
- `if` opens a conditional block, emitting a comparison opcode and a placeholder jump target; `else` and `endif` patch that target once the block's extent is known.
- `label` records the current bytecode offset under a name; `jump` emits a jump instruction referencing it.
- `routine` / `endroutine` collect their body into a separate bytecode segment, appended after the main program once compilation finishes; `call` emits a jump to that segment.

## Two-pass resolution

Labels, routine calls, and jump targets can reference names that haven't been seen yet (a `jump` to a `label` defined later in the file, a `call` to a `routine` defined below it). Rather than requiring forward declarations, the compiler emits **placeholder addresses** for these and records them as "unresolved." Once the whole file has been read and every `label`/`routine` position is known, a resolution pass walks the unresolved list and patches the real offsets into the bytecode.

This is also where routine bodies — compiled into their own separate buffers as they're encountered — get concatenated onto the end of the main bytecode stream, and their call sites get patched to point at the right offset.

## Output

The result is a flat `std::vector<uint8_t>` of bytecode plus a **string pool** (every string literal, deduplicated), a **const pool** (every integer and float literal, deduplicated — see [Bytecode Format](./bytecode-format.md#the-const-pool)), and a **variable table** (name → slot index). These get serialized to a `.bin` file by `BinaryProgram::save()` (`src/programfile.cpp`). If `--dbgsym` was passed, a parallel `.bin.dbg` file is written mapping variable slots, routine names, and offsets back to human-readable names — see [Debug Symbols](../debugging/debug-symbols.md).

For the exact byte-level format, see [Bytecode Format](./bytecode-format.md). For how it's executed, see [The Virtual Machine](./virtual-machine.md).