# The Virtual Machine

The VM is a stack machine. `run()` repeats: fetch the opcode at `PC`, `execute` it, move to the next instruction, until `HLT`, an error or the end of the program.

## State

| Piece | Purpose |
|---|---|
| `bytecode`, string pool, constant pool | The loaded program (read-only). |
| operand stack | `Variant` values; expressions and call arguments live here. |
| call stack | Return address and routine base for each active `CALL32`. |
| `PC` | Address of the next instruction. |
| routine base | Address the current routine starts at; added to relative jump targets. |
| memory | Variable and array storage, see [Memory & Garbage Collection](./memory.md). |
| `halt` flag | Set by `HLT` and by errors. |

## Execution model

- **Expressions** are postfix: operands are pushed, an operator pops two and pushes one.
- **Assignment** compiles to an expression followed by `POP <var>`.
- **Native calls:** `EXEC <index>` pops the function's arguments (last argument on top), runs the native function and pushes the result when it returns one.
- **Routine calls:** `CALL32 <addr>` pushes a frame and jumps; the body starts with `POP` instructions that store the arguments into the parameter variables; `RET` returns. A function's `return` pushes its value before `RET`.
- **Comparisons and branches** are fused compare-and-jump instructions (`JEQ32`...), which pop two values.
- **Loops** are built from those jumps; `repeat` uses hidden counters and `CPY` to publish the iterator.

## Errors

- A native function that throws is caught by `EXEC`: the VM prints `Function error` and the message, then halts.
- Any other exception during an instruction (array index errors, arithmetic on a string, modulo by zero) is caught by `execute`: the VM prints `Runtime error` and the message, then halts.
- Some malformed programs (for example popping an empty stack) are not checked; the compiler never generates them.

## Garbage collection

`run()` checks memory pressure every 1000 instructions and collects when needed. The debugger steps instructions without running the collector.
