# Conditionals

`if` / `elif` / `else` / `endif` blocks execute based on a comparison between two values.

```lumen
if age >= 18
    println 'Adult'
else
    println 'Minor'
endif
```

- Every `if` must be closed with `endif`.
- `else` and `elif` are optional.
- The condition uses one of the [comparison operators](./operators.md#comparison): `==`, `!=`, `>`, `<`, `>=`, `<=`.

## Chaining Conditions

Use `elif` (else-if) to chain multiple conditions without nesting:

```lumen
if mode == 1
    println 'Mode one'
elif mode == 2
    println 'Mode two'
else
    println 'Unknown mode'
endif
```

You can chain as many `elif` blocks as needed:

```lumen
if number % 15 == 0
    println 'Divisible by 15'
elif number % 3 == 0
    println 'Divisible by 3'
elif number % 5 == 0
    println 'Divisible by 5'
else
    println 'Not divisible by 3 or 5'
endif
```

## Inline Expressions in Conditions

Conditions can include arithmetic and other expressions directly:

```lumen
number = 15

if number % 15 == 0
    println 'Divisible by 15'
elif number % 3 == 0
    println 'Divisible by 3'
endif
```

See also [Loops](./loops.md), [FizzBuzz](../examples/fizzbuzz.md), and [Temperature Converter](../examples/temperature.md) for examples of conditionals in action.
