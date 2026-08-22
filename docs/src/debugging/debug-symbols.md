# Debug Symbols

Compiled `.bin` files are anonymous — variable names, routine names, and function names are all erased in favor of integer slots and offsets. Debug symbols restore that human-readable information for the [disassembler](./disassembler.md) and [debugger](./interactive-debugger.md) to use.

## Generating them

Pass `--dbgsym` alongside `--compile`:

```bash
lumen program.lmn --compile --dbgsym
```

This produces `program.lmn.bin` (the bytecode, as always) **and** `program.lmn.bin.dbg` (the symbol table).

## What's in the file

The `.dbg` file is plain text, written directly by the compiler (`src/compiler.cpp`) in three sections:

```
variables
<name> <slot index>
...
routines
<name>
<bytecode offset>
<bytecode length>
...
exec
<name> <function index>
...
```

- **`variables`** maps every variable name the compiler saw to its slot index in the VM's variable array.
- **`routines`** maps each routine name to where its compiled body starts in the bytecode and how long it is.
- **`exec`** maps built-in function names (`println`, `print`, `inputInt`, `inputStr`) to their `funcMap` index — see [Built-in Functions](../reference/builtin-functions.md).

## How it's consumed

Both `--disassemble` and `--debugger` look for a `.dbg` file next to the binary they're operating on (`<binary>.dbg`) and load it automatically if present, substituting names back in wherever the raw bytecode would otherwise show a bare integer. Without it, both tools still work — you just see slot numbers and offsets instead of the original identifiers.
