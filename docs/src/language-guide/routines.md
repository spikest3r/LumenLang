# Routines & Functions

Both are reusable blocks of code with parameters. A **routine** performs actions; a **function** also hands back a value with `return`.

```lumen
routine greet(name, times)
  repeat times
    println('Hi ' .. name)
  endrepeat
endroutine

function add(a, b)
  return a + b
endfunction

greet('Ann', 2)
total = add(2, 3)
println(add(add(1, 2), 4))     # 7
```

## Defining

- `routine name(p1, p2)` ... `endroutine`
- `function name(p1, p2)` ... `return <expression>` ... `endfunction`
- The parentheses may be empty or omitted for no parameters.
- Definitions can appear anywhere at the top level of a file, before or after the code that calls them. They cannot be nested.

## Calling

- A routine is a statement: `greet('Ann', 2)`.
- A function is used inside an expression: `x = add(1, 2)`, `println(add(1, 2))`, `'sum: ' .. add(1, 1)`.
- The argument count must match the definition (`argument count mismatch for 'greet': expected 2, got 1`).
- Using a routine as a value is a compile error (`'r' does not return a value`).

Every path through a function should end in a `return <value>`. `return` is only valid inside a function; a routine simply runs until `endroutine`.

## Parameters and globals

On entry the arguments are stored into global variables named after the parameters. There is no local scope: a parameter overwrites any global variable with the same name, and every variable assigned in a routine is visible everywhere.

Recursion works for values that were pushed before the recursive call, so this factorial is correct:

```lumen
function fact(n)
  if n <= 1
    return 1
  endif
  return n * fact(n - 1)
endfunction
println(fact(5))    # 120
```

but any variable read *after* a recursive call sees the value the callee left behind.

## Labels

Labels defined inside a routine are local to it, see [Labels & Jumps](./labels-and-jumps.md).

## Errors

`unknown function or routine: name` is reported for calls to something that is neither defined nor built in. Defining the same name twice (also across [imported](./imports.md) files) is an error.
