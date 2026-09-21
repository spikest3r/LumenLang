# Arrays

An array is a named, growable list of values. There is no declaration: writing to `name[index]` creates the array.

```lumen
scores[0] = 10
scores[1] = 20
scores[2] = scores[0] + scores[1]
println(scores[2])     # 30
```

## Indices and growth

- The index is any integer expression: `scores[i + 1]`, `scores[scores[0] - 9]`.
- Indices start at 0.
- Writing past the end grows the array; cells in between are filled with the integer `0`.
- Elements are untyped: an array can hold integers, floats and strings together.

```lumen
data[0] = 1.5
data[1] = 'two'
data[3] = 4        # data[2] becomes 0
```

## Reading

Reading a missing array, an index past the end, or a negative index is a runtime error: `Runtime error`, then `ARRREAD: index out of range` (or `array does not exist`), and the program stops. Writing to a negative index fails the same way (`ARRWRITE: negative index`).

## Notes

- Arrays and variables have separate namespaces: `a = 7` and `a[0] = 1` can coexist.
- Arrays are global and cannot be passed to or returned from routines, or assigned as a whole. Loop over the index instead.
- There is no built-in length. Keep the count in a variable.
- Array storage is managed by the [garbage collector](../architecture/memory.md).
- A program can use at most 256 distinct array names.

## Debugging

The [debugger](../debugging/interactive-debugger.md)'s `variables` command lists every array with its name, length and contents, and the [disassembler](../debugging/disassembler.md) shows array names on `ARRWRITE` / `ARRREAD`.
