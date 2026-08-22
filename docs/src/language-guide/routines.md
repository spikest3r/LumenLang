# Routines

A `routine` is a named, reusable block of code, invoked with `call`.

```lumen
routine hello

println 'Hello from a routine!'

endroutine

call hello
```

## Rules

- Every `routine` must be closed with `endroutine`.
- **Routines cannot be nested.** Defining a `routine` inside another `routine` is a compile error.
- **Routines take no parameters and return no value.** There's no argument list on `routine` or `call` — communication in and out of a routine happens entirely through variables, which are global and shared across the whole program.
- A routine can be `call`ed from anywhere in the file, including before its `routine ... endroutine` block appears — routine calls are resolved after the full file is compiled.

## Pattern: routines as functions over shared state

Because routines have no parameters, the idiomatic way to use them is to read and write well-known variable names, treating those variables as the routine's implicit "arguments" and "return value". The [Temperature Converter](../examples/temperature.md) example demonstrates this:

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

routine show
print 'Result: '
println result
endroutine

inputInt &temp
call c2f
call show
```

`c2f` and `f2c` both read `temp` and write `result`; `show` reads `result` back out. It's a manual convention, not something the language enforces — nothing stops a routine from touching a variable it "shouldn't", so keep routines small and their variable contracts obvious.
