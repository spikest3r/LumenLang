# FizzBuzz

```bash
lumen --examples fizzbuzz
```

```lumen
print('N=')
x = inputInt()
repeat x, n
n = n + 1
if n % 15 == 0
println('FizzBuzz')
elif n % 5 == 0
println('Buzz')
elif n % 3 == 0
println('Fizz')
else
println(n)
endif
endrepeat
```

`repeat x, n` runs the body `x` times and stores the iteration number (0, 1, 2, ...) in `n`; the body then rewrites `n` as `n + 1` so the printed numbers start at 1. The `if` / `elif` / `else` chain tests divisibility with `%`.

Touches: [loops](../language-guide/loops.md), [conditionals](../language-guide/conditionals.md), [operators](../language-guide/operators.md).

## Scan into Android

![QR for the fizzbuzz example](../images/qrcodes/fizzbuzz.png)
