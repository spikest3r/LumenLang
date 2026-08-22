# Variables & Values

Lumen is dynamically typed. A variable comes into existence the first time you assign to it — there's no separate declaration syntax.

```lumen
number = 42
result = number + 10

println result
```

## Value types

Under the hood, every value carried on the stack or stored in a variable is tagged with one of three types:

| Type | Description |
|---|---|
| Integer | Whole numbers, stored as 64-bit integers |
| Float | Floating-point numbers, stored as `double` |
| String | Text, delimited with single quotes |

You never annotate the type yourself — it's inferred from the literal or the result of an expression, and it can change across the lifetime of a variable, since Lumen re-tags the value on every assignment.

```lumen
x = 5        # integer
x = 'five'   # now a string — perfectly legal
```

### Float literals

A numeric literal is a float if it contains a decimal point or an exponent (`e`/`E`); otherwise it's an integer. This is a purely syntactic check (`isFloatLiteral()` in `src/helpers.cpp`) — it looks at how the literal is written, not its value:

```lumen
a = 3        # integer
b = 3.0      # float
c = 3e2      # float (300)
d = -3.5     # float
```

Integer and float literals are pooled together into the same deduplicated constant table — see [Bytecode Format](../architecture/bytecode-format.md#the-const-pool).

Arithmetic between an integer and a float promotes the result to float; `/` (division) always produces a float result regardless of operand types. See [Operators](./operators.md#arithmetic) for the full rules.

## Assignment

Assignment is a single `=`. The right-hand side can be a literal, another variable, or an arithmetic expression:

```lumen
a = 10
b = a * 2
c = a + b - 1
```

Negative numbers work as you'd expect:

```lumen
z = -20
```
