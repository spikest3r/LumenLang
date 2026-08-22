# Disassembler

`--disassemble` (`src/disassembler.cpp`) turns a compiled `.bin` file back into a readable instruction listing. It's read-only — it cannot be combined with `--compile` or `--run`.

```bash
lumen program.lmn --compile --dbgsym
lumen program.lmn.bin --disassemble
```

## Reading the output

```
===== Main =====
0x00000000: 03 02 00        | PUSH 0
0x00000003: 02 00           | POP i
0x00000005: 07 24 00 00 00  | CALL32 0x00000024
0x0000000a: 03 03 00        | PUSH i
...

===== Routine show =====
0x00000024: 03 03 00        | PUSH i
0x00000027: 04 01           | EXEC println
0x00000029: fe              | RET
```

Each line shows:

1. The bytecode offset, in hex — this is the address you'd use with the debugger's `breakpoint` or `pc` commands.
2. The raw operand bytes for that instruction.
3. The mnemonic (see [Opcode Reference](../reference/opcodes.md)) and its decoded operand.

Routine bodies are appended after the main program stream, and — if debug symbols are loaded — the disassembler prints a `===== Routine <name> =====` header right before the offset where each one begins.

## With and without debug symbols

If a matching `<binary>.dbg` file exists next to the file you're disassembling, it's loaded automatically (see [Debug Symbols](./debug-symbols.md)), and:

- `POP`/`PUSH` operands referencing variables show the variable's original name instead of its slot index.
- `CALL32` (or legacy `CALL`) shows the target routine's name instead of a bare offset.
- `EXEC` shows the built-in function's name (`println`, `inputInt`, ...) instead of its numeric index.

Without a `.dbg` file, you still get a complete, correct listing — just with numbers where names would be.
