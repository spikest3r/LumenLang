# Debug Symbols

Debug symbols map the numbers in bytecode back to names. They are embedded in the `.bin` (no separate file) and are on by default; `--no-debug` leaves them out.

## Format

A text block at the end of the `.bin`, in four sections:

```text
variables
<name> <slot index>
routines
<name>
<start offset>
<length>
exec
<name> <function index>
arrays
<name> <array index>
```

- **variables**: variable slot for each name (including hidden `@cnt_...` loop counters).
- **routines**: name, start address and length of each routine and function.
- **exec**: every built-in function and its `EXEC` index, e.g. `println 1`.
- **arrays**: array number for each array name.

Older binaries without the `arrays` section still load.

## What uses them

- The [disassembler](./disassembler.md) prints `PUSH x`, `EXEC println`, `ARRWRITE scores` and `===== Routine add =====` headers.
- The [debugger](./interactive-debugger.md) prints variable and array names in `variables`.

Without symbols the same output shows numbers: `[0] 1`, `EXEC fn[1]`, `ARRWRITE arr[0]`.
