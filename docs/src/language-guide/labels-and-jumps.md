# Labels & Jumps

`label <name>` marks a position and `jump <name>` transfers control to it unconditionally. The pair below is an infinite loop, see the [Labels & Jumps Demo](../examples/jump.md).

```lumen
label loop
println('Hello, world!')
jump loop
```

Jumps can go forward or backward:

```lumen
i = 0
label top
i = i + 1
if i < 3
  jump top
endif
println(i)      # 3
```

## Scope

Labels in the main program are visible to the whole main program (including code brought in by [`import`](./imports.md)). Labels inside a routine or function are local to it. Jumping to a label in another scope is a compile error (`Label 'x' is not defined in global scope`).

## Stopping the program

`halt` stops execution immediately:

```lumen
println('done')
halt
println('never printed')
```

The program also ends when the last statement has run.
