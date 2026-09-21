# Labels & Jumps Demo

```bash
lumen --examples infinite-loop
```

This writes `infinite.lmn`:

```lumen
label loop
println('Hello, world!')
jump loop
```

`label loop` marks a position; `jump loop` goes back to it, forever. Stop it with Ctrl+C. See [Labels & Jumps](../language-guide/labels-and-jumps.md) for conditional loops built from the same two statements.

## Scan into Android

![QR for the labels-and-jumps example](../images/qrcodes/jump.png)
