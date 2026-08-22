# The Virtual Machine

The Lumen VM (`src/vm.cpp`) is a **stack machine**: instructions operate on an operand stack rather than registers. It's a straightforward fetch-decode-execute loop.

## Execution state

Two structs hold VM state (`include/types.h`). `VMProgramData` holds the compiled program itself (read-only during execution):

| Field | Type | Purpose |
|---|---|---|
| `bytecode` | `std::vector<uint8_t>` | The instruction stream |
| `stringPool` | `std::vector<std::string>` | Pooled string literals |
| `constPool` | `std::vector<double>` | Pooled integer + float constants |
| `variableCount` | `int` | Number of variable slots to allocate |

`VMExecutionData` holds the mutable state that changes as the program runs:

| Field | Type | Purpose |
|---|---|---|
| `variables` | `std::vector<Variant>` | Flat variable storage, one slot per variable |
| `stack` | `std::vector<Variant>` | Operand stack — values being computed with |
| `pcStack` | `std::vector<CallFrame>` | Return-address stack for routine calls, each frame holding a return `PC` and the caller's `routineBase` |
| `PC` | `int` | Program counter — index into the bytecode stream |
| `routineBase` | `int` | Base offset of the currently executing routine (`0` for the main program), added to jump targets |
| `halt` | `bool` | Set by `HLT` to stop the main loop |

## The main loop

```cpp
int run(VMProgramData* progData) {
    VMExecutionData execData;
    execData.variables.resize(progData->variableCount);

    while (true) {
        int result = execute(progData, &execData);
        if (execData.halt || result == -1) break;
        execData.PC = result;
    }
    return 0;
}
```

`execute()` decodes a single instruction, performs its effect, and returns the *next* `PC` value (or `-1` on error). Most instructions simply return `PC + offset` (fall through to the next instruction); jumps, conditional jumps, and calls instead return an arbitrary target address — usually `routineBase + <encoded offset>` — which is how control flow works. There's no separate branch-prediction or block structure at runtime, just PC reassignment.

## How each construct compiles down

| Language construct | Bytecode behavior |
|---|---|
| `x = expr` | Evaluate `expr` by pushing operands and applying arithmetic opcodes (`ADD`/`SUB`/...), then pop the result into `variables[x]` |
| `println` / `print` / `inputInt` / `inputStr` / ... | `EXEC` (`0x04`) dispatches through a native function table — see [Built-in Functions](../reference/builtin-functions.md) |
| `if <a> <cmp> <b>` | Push `a` and `b`, then a comparison opcode (`JEQ32`/`JGR32`/...) that pops both and jumps past the block **if the comparison is false** |
| `label` / `jump` | `label` is purely a compile-time bookmark; `jump` compiles to `JUMP32` (`0x06`), an unconditional PC reassignment |
| `routine` / `call` / `endroutine` | `call` compiles to `CALL32` (`0x07`), which pushes a return frame onto `pcStack` and jumps to the routine's offset; `endroutine` compiles to `RET` (`0xFE`), which pops `pcStack` and jumps back |
| `..` (string concat) | `JOIN` (`0xAA`) pops a count and that many strings off the stack, concatenates them, and pushes the result |

See [Opcode Reference](../reference/opcodes.md) for the full opcode table, including the legacy 8-bit `CALL`/`JUMP`/`JEQ`-style opcodes that the VM still executes but the compiler no longer emits.

## Native functions

`print`, `println`, `inputInt`, and `inputStr` aren't opcodes of their own — they're entries in `funcMap` (`src/vmfuncmap.cpp`), a lookup table from a small integer index to a C++ lambda. The `EXEC` opcode (`0x04`) just looks up the index and calls the corresponding lambda with the current stack and variable storage. This is a deliberately extensible design: adding a new built-in function means adding one entry to `funcMap` plus a matching entry to the compiler's `funcList`, without touching the opcode set at all.

On embedded targets (Pico), the equivalent table is `funcTable` — a plain array of `NativeFn` function pointers rather than lambdas. `EXEC` operands in the `0xD0`–`0xFF` range are reserved on these targets for application-specific native functions (e.g. GPIO calls), mapped onto the tail end of `funcTable` at a fixed offset from the base opcode; this range is Pico-specific and not part of the desktop VM's dispatch.

## Halting

Execution stops when the VM reaches `HLT` (`0xFF`), which the compiler appends to the end of the main bytecode stream after every line of source has been compiled.

## Debug and disassembly variants

`src/vm.cpp` implements the plain execution loop described above. Two related tools reuse the same `execute()` function but wrap it differently:

- `run_debug()` (`src/debugvm.cpp`) — the interactive [debugger](../debugging/interactive-debugger.md), which steps through `execute()` one instruction at a time and inspects the same `stack`/`variables`/`PC` state between steps.
- `disassemble()` (`src/disassembler.cpp`) — doesn't execute anything; it statically walks the bytecode stream opcode by opcode and prints a human-readable listing. See [Disassembler](../debugging/disassembler.md).
