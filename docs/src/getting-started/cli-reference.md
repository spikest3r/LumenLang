# CLI Reference

```
lumen <file> [options]
```

## Special first arguments

These take the place of a file name and short-circuit everything else:

| Argument | Description |
|---|---|
| `--introduction` | Write a starter `helloworld.lmn` to the current directory |
| `--examples` | List all built-in examples |
| `--examples <name>` | Write the named example to `<name>.lmn` |
| `--help` | Print usage and exit |
| `--version` | Print version, git branch, commit, and build date |

## File flags

Everything else takes `<file>` as the first argument, followed by any of:

| Flag | Description |
|---|---|
| `--verbose` | Print extra compiler/VM diagnostics, including the raw compiled bytecode |
| `--compile` | Compile the source file to `<file>.bin` |
| `--run` | Execute compiled bytecode |
| `--disassemble` | Disassemble a compiled `.bin` file into readable instructions |
| `--dbgsym` | Emit a `<file>.bin.dbg` debug symbols file alongside the bytecode |
| `--debugger` | Run under the interactive [debugger](../debugging/interactive-debugger.md) |

## Default behavior

If you pass **no** `--compile`, `--run`, or `--disassemble` flag, Lumen compiles and runs the file in one shot:

```bash
lumen script.lmn
# equivalent to:
lumen script.lmn --compile --run
```

## Flag combination rules

Lumen enforces a few sane combinations and will refuse to run with an error otherwise:

- **`--disassemble` is exclusive.** It cannot be combined with `--compile` or `--run` — disassembling reads an existing `.bin` file, it doesn't produce or execute one.
- **`--debugger` requires `--run`.** You can't compile-only into the debugger; the debugger attaches to execution.
- **`--dbgsym` requires `--compile`.** Debug symbols are only generated as part of compilation.