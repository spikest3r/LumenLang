# Disassembler

```bash
lumen program.lmn.bin --disassemble
```

Prints the address, raw bytes and mnemonic of every instruction. With [debug symbols](./debug-symbols.md) operands are shown by name.

```text
===== Main =====
0x00000000: 03 02 00        | PUSH 2
0x00000003: 03 02 01        | PUSH 3
0x00000006: 07 0e 00 00 00  | CALL32 0x0000000e
0x0000000b: 04 01           | EXEC println
0x0000000d: ff              | HLT

===== Routine add =====
0x0000000e: 02 00           | POP b
0x00000010: 02 01           | POP a
0x00000012: 03 03 01        | PUSH a
0x00000015: 03 03 00        | PUSH b
0x00000018: a0              | ADD
0x00000019: fe              | RET
0x0000001a: fe              | RET
```

- Routine bodies are appended after the main program. Each starts with a `===== Routine <name> =====` header when symbols are present.
- `CALL32` and the jumps show their target address.
- `PUSH` shows the value, string, constant or variable it pushes.
- `EXEC` shows the built-in's name; `ARRWRITE` / `ARRREAD` show the array's name.
- The same output is available inside the debugger with `disassemble`.

Opcodes are listed in the [Opcode Reference](../reference/opcodes.md).
