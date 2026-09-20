# Variables & Values

## Assignment

```lumen
count = 10
price = 2.5
name = 'Lumen'
```

There are no declarations: the first assignment creates the variable. Names start with a letter or `_` and continue with letters, digits or `_`. Keywords cannot be used as names. Names starting with `@` are reserved for the compiler.

## Types

| Type | Literal | Notes |
|---|---|---|
| Integer | `42`, `-7` | 64-bit signed. |
| Float | `3.14` | Double precision. |
| String | `'text'` or `"text"` | See [Strings](./strings.md). |

A variable's type is whatever it was last assigned; it can change freely:

```lumen
x = 5
x = 'five'
```

Reading a variable that was never assigned gives the integer `0`.

### Float literals

A number containing a `.` is a float. `3.0` is a float; `3` is an integer. Arithmetic between an integer and a float produces a float. See [Operators](./operators.md#arithmetic).

## Everything is global

All variables live in one global scope. Routine and function parameters and any variable assigned inside a routine are ordinary globals too, see [Routines & Functions](./routines.md).

## Limits

Variables, arrays, string literals and numeric constants are each addressed by a single byte in the bytecode, so one program can use at most 256 of each. Exceeding this is not detected by the compiler. Each `repeat` nesting level also uses one hidden counter variable, named `@cnt_<n>_<depth>`.

## Printing values

`println` prints integers as they are and floats with up to six significant digits (`3.5`, `0.333333`, `3` for `3.0`). Joining a float into a string with `..` uses six decimals (`2.500000`).
