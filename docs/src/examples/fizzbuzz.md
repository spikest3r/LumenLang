# FizzBuzz

Generate it:

```bash
lumen --examples fizzbuzz
```

```lumen
N = 0

print 'N='
inputInt &N
println ''

i = 1

label loop

i15 = i % 15

if i15 == 0
    println 'FizzBuzz'
else
    i3 = i % 3

    if i3 == 0
        println 'Fizz'
    else
        i5 = i % 5

        if i5 == 0
            println 'Buzz'
        else
            println i
        endif
    endif
endif

i = i + 1

if i <= N
    jump loop
endif
```

The classic FizzBuzz, and the best single example of how the language pieces fit together:

- A `label loop` / conditional `jump loop` pair drives iteration from `i = 1` up to `N` (see [Labels & Jumps](../language-guide/labels-and-jumps.md)).
- `%` (modulo) checks divisibility by 15, then 3, then 5.
- Three levels of nested `if`/`else`/`endif` stand in for the `elseif` chain Lumen doesn't have (see [Conditionals](../language-guide/conditionals.md)).

Note the divisibility-by-15 check runs first — this is the same "check the most specific case first" trick FizzBuzz solutions need in any language, since 15 is also divisible by 3 and 5.

## Scan into Android

![QR for the fizzbuzz example](../images/qrcodes/fizzbuzz.png)

See [Lumen on Android](../lumen-in-apps/lumen-on-android.md) for how the scan-and-run pipeline works.
