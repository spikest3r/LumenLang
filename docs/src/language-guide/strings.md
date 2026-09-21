# Strings

Strings are written with single or double quotes:

```lumen
a = 'hello'
b = "world"
println(a .. ' ' .. b)
```

## Escape sequences

| Sequence | Result |
|---|---|
| `\n` | newline |
| `\t` | tab |
| `\r` | carriage return |
| `\\` | backslash |
| `\'` | single quote |
| `\"` | double quote |

Any other backslash sequence is kept as written. Because `\t` and `\n` are escapes, write Windows paths with forward slashes or doubled backslashes (`'C:\\temp'`).

```lumen
println('it\'s')
println("say \"hi\"\nbye")
```

## Joining

`..` joins values. Integers are printed as-is and floats with six decimals:

```lumen
println('total: ' .. 2.5)   # total: 2.500000
```

## String manipulation

The standard library has `strlen`, `substr`, `strfind`, `strcase`, `trim` and the conversions `str2int`, `int2str`, `str2float`, `float2str`. See [Standard Library](./standard-library.md).

## Comparing

`==`, `<` and friends compare two strings character by character. Comparing a string to a number treats the string as `0`.
