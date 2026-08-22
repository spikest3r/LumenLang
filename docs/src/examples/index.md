# Examples Overview

Lumen ships example programs baked directly into the `lumen` binary (see `include/examples.h`). You don't need to find them on disk — generate any of them with:

```bash
lumen --examples <name>
```

| Example | Generates | Description |
|---|---|---|
| `age` | `age.lmn` | Age calculator — reads a birth year, computes age |
| `fizzbuzz` | `fizzbuzz.lmn` | Classic FizzBuzz up to a user-supplied `N` |
| `temperature` | `temperature.lmn` | Celsius ↔ Fahrenheit converter using routines |
| `infinite-loop` | `jump.lmn` | Minimal `label`/`jump` demonstration |

Two additional programs — `math.lmn` and `paint.lmn` — live in the repository's `examples/` folder as source but aren't wired into `lumen --examples`; you can still read them directly from the repo for more arithmetic and expression examples.

Each example is broken down in its own page in this chapter, in increasing order of what language features it touches. Every example page also carries a QR next to its source. It can be scanned directly into [Lumen on Android](../lumen-in-apps/lumen-on-android.md) without needing to compile anything on-device.
