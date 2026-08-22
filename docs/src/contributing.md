# Contributing

LumenLang is a solo, exploratory project, but issues, pull requests, and questions are welcome on [GitHub](https://github.com/spikest3r/LumenLang).

## Before you send a PR

1. **Build it.** Follow [Installation & Building](./getting-started/installation.md) — make sure `cmake .. && make -j$(nproc)` succeeds cleanly.
2. **Run the test suite.**
   ```bash
   ./test.sh
   ```
   This exercises the compiler and VM end-to-end. Any change to `src/compiler.cpp`, `src/vm.cpp`, `src/tokenizer.cpp`, or the bytecode format should leave this passing.
3. **Keep the disassembler and debug symbols in sync.** If you add or change an opcode, update `disassemblyMap` in `src/disassembler.cpp` and `getOpCodeOffset()` in `src/helpers.cpp` together — a mismatch between the two silently corrupts disassembly output. See [Opcode Reference](./reference/opcodes.md).

## Where things live

| Area | Files |
|---|---|
| Tokenizer | `src/tokenizer.cpp`, `include/tokenizer.h` |
| Compiler | `src/compiler.cpp`, `src/compiler_math.cpp`, `include/compiler.h` |
| Virtual machine | `src/vm.cpp`, `include/vm.h` |
| Native functions | `src/vmfuncmap.cpp` |
| Binary `.bin` format | `src/programfile.cpp`, `include/programfile.h` |
| Disassembler | `src/disassembler.cpp`, `include/disassembler.h` |
| Interactive debugger | `src/debugvm.cpp` |
| Bundled examples | `include/examples.h`, `examples/*.lmn` |