# Operators

## Arithmetic

| Operator | Meaning |
|---|---|
| `+` | Addition |
| `-` | Subtraction |
| `*` | Multiplication |
| `/` | Division — **always produces a float result**, even for two integers |
| `%` | Modulo |
| `^` | Exponentiation |

If either operand of `+`, `-`, `*`, `%`, or `^` is a [float](./variables-and-values.md#float-literals), the result is a float; if both operands are integers, the result stays an integer. `/` is the one exception — it always yields a float, so `7 / 2` is `3.5`, not `3`:

```lumen
a = 7
b = 2
c = a / b
println c   # 3.500000
```

Parentheses can be used to control evaluation order:

```lumen
z = -20
a = z
b = 30
c = a + b
d = c * a + b
e = d / (a + b)
println e
```

## Comparison

Used in [conditionals](./conditionals.md) and [conditional jumps](./labels-and-jumps.md). Comparisons read both operands as `double`, so an integer and a float compare correctly against each other (`5 == 5.0` is true):

| Operator | Meaning |
|---|---|
| `==` | Equal |
| `!=` | Not equal |
| `>` | Greater than |
| `<` | Less than |
| `>=` | Greater than or equal |
| `<=` | Less than or equal |

## String concatenation

`..` concatenates two values into a string:

```lumen
name = 'Ryan'
text = 'Hello, ' .. name

println text
```

See [Strings](./strings.md) for more.
