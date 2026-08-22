# Conditionals

`if` / `else` / `endif` blocks execute based on a comparison between two values.

```lumen
if age >= 18
    println 'Adult'
else
    println 'Minor'
endif
```

- Every `if` must be closed with `endif`.
- `else` is optional.
- The condition uses one of the [comparison operators](./operators.md#comparison): `==`, `!=`, `>`, `<`, `>=`, `<=`.

## Nesting

`if`/`endif` blocks nest freely — there's no `elseif`, so a chain of conditions is written as nested `if`/`else` blocks:

```lumen
if mode == 1
    println 'Mode one'
else
    if mode == 2
        println 'Mode two'
    else
        println 'Unknown mode'
    endif
endif
```

This is exactly the pattern used in the [Temperature Converter](../examples/temperature.md) example. See also [FizzBuzz](../examples/fizzbuzz.md), which nests three levels deep to check divisibility by 15, 3, and 5.
