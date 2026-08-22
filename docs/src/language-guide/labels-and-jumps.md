# Labels & Jumps

Lumen has no dedicated loop keywords (`while`, `for`) — loops are built from `label` and `jump`, the same primitives the underlying [bytecode](../architecture/bytecode-format.md) exposes directly.

## Unconditional jump

```lumen
label repeat
println 'Hello, world!'
jump repeat
```

`label <name>` marks a position in the program. `jump <name>` transfers control to it unconditionally. The pair above is an infinite loop — see the [Labels & Jumps Demo](../examples/jump.md) example.

## Conditional jump

Combine `jump` with an `if` block to build a real loop with an exit condition:

```lumen
i = 0

label loop

println i

i = i + 1

if i < 10
    jump loop
endif
```

This counts from 0 to 9. The loop body runs, `i` increments, and the `if` decides whether to jump back to `label loop` or fall through and end the program.

## Scope

Labels are program-global — a `jump` can target any `label` in the file, not just ones in the same block. This is what makes them powerful (and occasionally easy to misuse): there's no structural nesting enforced between a `label` and the `jump`s that target it, unlike `if`/`endif` or `routine`/`endroutine`.
