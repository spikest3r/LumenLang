# Temperature Converter

Generate it:

```bash
lumen --examples temperature
```

```lumen
temp = 0
result = 0

routine c2f
result = temp * 9 / 5 + 32
endroutine

routine f2c
result = temp - 32
result = result * 5 / 9
endroutine

routine ask
print 'Temparature: '
endroutine

routine show
print 'Result: '
println result
endroutine

println '1. C to F'
println '2. F to C'
print 'Select mode '
mode = 0
inputInt &mode
if mode == 1
    call ask
    inputInt &temp
    call c2f
    call show
elif mode == 2
    call ask
    inputInt &temp
    call f2c
    call show
else
    println 'Incorrect mode'
endif
```

The most feature-complete example in the box. It combines:

- **Four routines** (`c2f`, `f2c`, `ask`, `show`) that communicate purely through the shared variables `temp` and `result` — see [Routines](../language-guide/routines.md) for why this pattern exists.
- **`elif` chain** to build a three-way menu (`mode == 1`, `mode == 2`, anything else) — see [Conditionals](../language-guide/conditionals.md).
- **`inputInt`** used twice: once for the menu selection, once for the temperature value itself.

It's a good template to copy from when you want a small menu-driven Lumen program with reusable logic.

## Scan into Android

![QR for the temperature example](../images/qrcodes/temperature.png)

See [Lumen on Android](../lumen-in-apps/lumen-on-android.md) for how the scan-and-run pipeline works.
