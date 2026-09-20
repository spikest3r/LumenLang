# CLI Reference

```text
lumen (file) [options]
```

The file comes first, options after it. With no options the file is compiled and then run.

| Option | Meaning |
|---|---|
| `--compile` | Compile the source file to `<file>.bin`. |
| `--run` | Run a compiled program. Used alone, `file` must be a `.bin`; combined with `--compile`, the freshly written `<file>.bin` is run. |
| `--disassemble` | Print the disassembly of a `.bin`. Cannot be combined with `--compile` or `--run`. |
| `--debugger` | Run under the [interactive debugger](../debugging/interactive-debugger.md). Cannot be combined with `--compile` alone. |
| `--no-debug` | Do not embed debug symbols in the compiled `.bin` (smaller file, no names in the disassembler or debugger). |
| `--verbose` | Print prescan tokens, the emitted bytecode and execution progress. |

Standalone commands (no file needed):

| Command | Meaning |
|---|---|
| `--help` | Print usage. |
| `--version` | Print the language version, git branch, commit and build date. |
| `--introduction` | Write a starter script `helloworld.lmn` to the current directory. |
| `--examples` | List the bundled examples. |
| `--examples <name>` | Write the named example to disk (`age.lmn`, `infinite.lmn`, `temperature.lmn`, `fizzbuzz.lmn`). |

## Typical workflows

```bash
lumen script.lmn                       # compile + run
lumen script.lmn --compile             # writes script.lmn.bin
lumen script.lmn.bin --run             # run the compiled program
lumen script.lmn.bin --disassemble     # inspect the bytecode
lumen script.lmn --debugger            # compile, then debug
lumen script.lmn --compile --no-debug  # release build without symbols
```

## Errors

Compile errors name the file and line and stop the build:

```text
In file script.lmn
  - Error on line 3
    >>> argument count mismatch for 'println': expected 1, got 0
Compilation failed!
```

Runtime errors print `Runtime error` (or `Function error` when a built-in fails) followed by a message, and the program stops. The process exit code is still 0.
