# Compiler Pipeline

```text
 source (.lmn)
     │
     ▼
 read all lines ──► prescan routines/functions (names, arg counts, indices)
     │
     ▼
 per line: protect string literals ──► tokenize ──► statement dispatch
     │                                                  │
     │                              expressions: shunting-yard ──► RPN ──► bytecode
     ▼
 finalize: HLT, resolve jumps, append routine bodies, patch calls, write debug data
     │
     ▼
 Program (bytecode + string pool + constant pool + debug data) ──► .bin
```

The code lives in `src/compiler/` (with `src/tokenizer.cpp` and `src/helpers.cpp`):

| File | Role |
|---|---|
| `compiler.cpp` | Entry points (`compileFromFile`, `compileFromText`, `compileFromStream`); reads lines, runs the prescan, drives the line loop and finalization. |
| `compiler_statement.cpp` | `compileLine`: one state machine for every statement (assignment, `if`, loops, `routine`, `function`, `return`, `import`, calls, `label`/`jump`, `halt`). |
| `exprparse.cpp` | Expression compiler: tokenizer, shunting-yard, RPN evaluation to bytecode (operators, calls, array reads, `&`/`*`). |
| `compiler_helpers.cpp` | Routine prescan, error printing, `pushToStack`. |
| `compiler_tables.cpp` | Operator, keyword and built-in function tables (`funcList`). |
| `compiler_finalize.cpp` | Jump/call resolution, routine layout, debug-symbol emission. |

## Stages

1. **Prescan.** Every `routine` / `function` header is found first, so a call can appear before the definition. Each gets an index, an argument count and a "returns a value" flag. Names are unique across imported files.
2. **String protection.** String literals are replaced by placeholders so `#`, `..`, quotes and operators inside them never reach the tokenizer; escapes are decoded when the literal is turned into a pool entry.
3. **Tokenizing.** `&x` and `*r` become single tokens when they sit in an operand position; the same characters are binary operators (`*`) elsewhere.
4. **Statements.** `compileLine` keeps per-file state (`CompileState`): open blocks, labels, unresolved jumps and calls, the routine being compiled. Loops use hidden counter variables named `@cnt_<context>_<depth>`.
5. **Expressions.** Conditions and right-hand sides are compiled to postfix code that leaves one value on the stack. Function calls and array reads are compiled in place.
6. **Imports.** `import` recursively compiles the other file with the *same* state and data, so labels, routines, pools and variable slots are shared and the code lands in the main stream.
7. **Finalization.** Appends `HLT`, patches jumps to labels, lays out routine bodies after the main code, patches `CALL32` targets and, unless `--no-debug`, builds the [debug symbols](../debugging/debug-symbols.md).

## Diagnostics

Errors are printed as `In file <file>` / `Error on line N` / `>>> message`. A failed import adds `Script compilation has failed` for the importing file.
