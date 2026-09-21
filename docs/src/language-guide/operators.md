# Operators

## Arithmetic

| Operator | Meaning | Notes |
|---|---|---|
| `+` `-` `*` | add, subtract, multiply | Integer result when both operands are integers, float otherwise. |
| `/` | divide | Always a float division: `7 / 2` is `3.5`. |
| `%` | remainder | Integer remainder for integers, `fmod` for floats. Integer `% 0` is a runtime error. |
| `^` | power | `2 ^ 10` is `1024`; `2 ^ 0.5` is a float. Right associative. |
| `-x` | negation | Binds tighter than `^`: `-2 ^ 2` is `4`. |
| `( )` | grouping | |

Precedence, high to low: unary `-`, `^`, `* / %`, `+ -`, `..`.

```lumen
println(2 ^ 3 ^ 2)   # 512
println(10 - 2 - 3)  # 5
println(7 % 3)       # 1
```

Arithmetic on a string literal is rejected at compile time (`type error: arithmetic operator '+' cannot be applied to a string literal`). Arithmetic on a string *variable* is caught at run time: `Runtime error`, then `type error: arithmetic on a string value`, and the program stops.

## String join

`..` joins two values into a string; numbers are converted. It has the lowest precedence, so `'n=' .. 1 + 2` gives `n=3`.

## Comparison

`==`, `!=`, `<`, `>`, `<=`, `>=` are used in [`if`, `elif` and `while`](./conditionals.md). Numbers compare numerically (`5 == 5.0` is true) and two strings compare lexicographically. A condition holds exactly one comparison; there is no `and`, `or` or `not`, so nest `if` blocks instead.

## Reference operators

`&x` and `*r` are covered in [References & Dereferencing](./references.md).
