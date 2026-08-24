# Loops

Lumen provides two primary loop constructs: `while` loops for condition-based iteration, and `repeat` loops for fixed-count iteration.

## While Loop

A `while` loop repeats a block as long as a condition is true:

```lumen
i = 0

while i < 10
    println i
    i = i + 1
endwhile
```

- The condition is checked at the start of each iteration.
- The loop exits when the condition becomes false.
- If the condition is false initially, the loop body never executes.

## Repeat Loop

A `repeat` loop repeats a block a fixed number of times:

```lumen
repeat 5
    println 'Hello'
endrepeat
```

### Repeat with Iterator

Use the optional iterator syntax to automatically manage a loop variable:

```lumen
repeat 5, i
    println i
endrepeat
```

This prints `0 1 2 3 4` (the iterator starts at 0 and increments each iteration).

The iterator is equivalent to:

```lumen
i = 0
repeat 5
    println i
    i = i + 1
endrepeat
```

## Loop Control

### Break

Use `break` to exit a loop early:

```lumen
i = 0

while i < 100
    if i == 5
        break
    endif
    println i
    i = i + 1
endwhile
```

### Continue

Use `continue` to skip to the next iteration:

```lumen
repeat 10, i
    if i % 2 == 0
        continue
    endif
    println i
endrepeat
```

This prints only the odd numbers from 0 to 9.

## Combining Loops and Conditionals

Loops and conditionals work seamlessly together. Here's FizzBuzz using a repeat loop with inline condition expressions:

```lumen
repeat 15, i
    if i % 15 == 0
        println 'FizzBuzz'
    elif i % 3 == 0
        println 'Fizz'
    elif i % 5 == 0
        println 'Buzz'
    else
        println i
    endif
endrepeat
```

See [Conditionals](./conditionals.md) for more details on `if`, `elif`, and `else`.

## Legacy: Labels and Jumps

Before loops were added to Lumen, iteration was handled with labels and jumps. This approach is still supported but `while` and `repeat` loops are preferred:

```lumen
i = 0

label loop

println i

i = i + 1

if i < 10
    jump loop
endif
```

See [Labels & Jumps](./labels-and-jumps.md) for more details.
