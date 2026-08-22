# Labels & Jumps Demo

Generate it:

```bash
lumen --examples infinite-loop
```

```lumen
label repeat
println 'Hello, world!'
jump repeat
```

The smallest possible demonstration of Lumen's looping primitive. `label repeat` marks a point in the program; `jump repeat` transfers control back to it — forever, in this case, since there's no condition around the jump. Run it and stop it with `Ctrl+C`.

See [Labels & Jumps](../language-guide/labels-and-jumps.md) for how to turn this into a bounded loop with an exit condition.

## Scan into Android

![QR for the labels-and-jumps example](../images/qrcodes/jump.png)

See [Lumen on Android](../lumen-in-apps/lumen-on-android.md) for how the scan-and-run pipeline works.

### Note

This particular program never halts on its own (`Ctrl+C` on desktop) — on Android, use the runtime's [cooperative cancellation](../lumen-in-apps/lumen-on-android.md#cancellation) to stop it instead.
