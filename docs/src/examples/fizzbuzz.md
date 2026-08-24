# FizzBuzz

Generate it:

```bash
lumen --examples fizzbuzz
```

```lumen
print 'N='
inputInt &x
repeat x, n
n = n + 1
if n % 15 == 0
    println 'FizzBuzz'
elif n % 5 == 0
    println 'Buzz'
elif n % 3 == 0
    println 'Fizz'
else
    println n
endif
endrepeat
```

The classic FizzBuzz, now written with modern Lumen loop and condition syntax:

- `repeat x, n` creates a loop that runs `x` times with an iterator `n` that starts at 0 (see [Loops](../language-guide/loops.md)).
- Since the iterator starts at 0, we increment it to get 1-indexed counting: `n = n + 1`.
- `%` (modulo) checks divisibility, using `elif` chains instead of nested blocks (see [Conditionals](../language-guide/conditionals.md)).

Note the divisibility-by-15 check runs first — this is the "check the most specific case first" trick FizzBuzz solutions need in any language, since 15 is also divisible by 3 and 5.

## Scan into Android

![QR for the fizzbuzz example](../images/qrcodes/fizzbuzz.png)

See [Lumen on Android](../lumen-in-apps/lumen-on-android.md) for how the scan-and-run pipeline works.
