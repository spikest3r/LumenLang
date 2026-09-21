# References & Dereferencing

A **reference** is the address (slot number) of a variable.

| Syntax | Meaning |
|---|---|
| `&x` | the reference to variable `x` |
| `*r` | the current value of the variable that `r` refers to |

```lumen
x = 10
r = &x
println(*r)      # 10
x = 20
println(*r)      # 20
println(*r + 1)  # 21
```

A reference is stored as a plain integer, so `println(r)` prints a slot number. `*r` can be used anywhere an expression is allowed (assignments, arguments, conditions):

```lumen
if *r == 20
  println('twenty')
endif
```

## Rules

- Assigning *through* a reference (`*r = 5`) is not supported and is a compile error.
- Dereferencing a reference to a variable that was never assigned stops the program.
- References are how built-ins return extra values. `httpRequest` writes the response body into the variable you pass with `&`, see [Standard Library](./standard-library.md#http).
