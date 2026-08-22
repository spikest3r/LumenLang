# Strings

Strings are delimited with **single quotes**.

```lumen
message = 'Hello, Lumen!'
println message
```

## Concatenation

Use `..` to join strings (or a string and another value) together:

```lumen
name = 'Ryan'
text = 'Hello, ' .. name

println text
```

## No apostrophes inside strings

Because a single quote (`'`) is the string delimiter, you can't put a literal apostrophe inside a string — the tokenizer would read it as the end of the string. There's no escape sequence for this yet, so the convention used in the examples is to substitute a backtick where an apostrophe would go:

```lumen
print 'What`s your name? '
```

This is printed exactly as written — the backtick is **not** converted to an apostrophe, it's just a visual stand-in the source code uses to avoid breaking the string literal. Keep this in mind if you're generating output that should read naturally; there is currently no way to produce a real `'` character inside a string.

## Converting to and from numbers

Strings don't automatically convert to numbers (or vice versa) in arithmetic or comparisons — use one of the four conversion built-ins, all of which take the value first and the destination variable second:

```lumen
n = 0
str2int '42' &n        # n = 42 (int)

f = 0.0
str2float '3.14' &f    # f = 3.14 (float)

s1 = ''
int2str 42 &s1          # s1 = '42'

s2 = ''
float2str 3.5 &s2       # s2 = '3.500000'
```

`str2int`/`str2float` fall back to `0`/`0.0` on unparseable input rather than raising an error. See [Built-in Functions](../reference/builtin-functions.md) for the exact semantics.

## String Manipulation

| Function | Description |
|---|---|
| `strlen s, &out` | Length of a string |
| `substr s, start, len, &out` | Extract a substring |
| `strfind s, needle, &out` | Index of the first occurrence of `needle` in `s`, or `-1` if not found |
| `strcase s, upper, &out` | Convert case: `upper = 1` for uppercase, `0` for lowercase |
| `trim s, &out` | Strip leading and trailing whitespace |
