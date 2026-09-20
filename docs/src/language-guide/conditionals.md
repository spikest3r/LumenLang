# Conditionals

```lumen
if score >= 90
  println('A')
elif score >= 80
  println('B')
else
  println('C or below')
endif
```

`elif` and `else` are optional and `elif` can repeat. Every `if` ends with `endif`.

## Conditions

A condition is one comparison between two expressions: `==`, `!=`, `<`, `>`, `<=`, `>=`. Both sides can be full expressions, including function calls, array reads and dereferences:

```lumen
if strlen(name) > 3
  println('long name')
endif
if scores[0] == *r
  println('match')
endif
```

Numbers compare numerically and two strings compare lexicographically. A bare value (`if x`) and boolean operators (`and`, `or`, `not`) do not exist.

## Nesting

Blocks nest to any depth. Use nested `if`s in place of `and`:

```lumen
if age >= 18
  if hasTicket == 1
    println('welcome')
  endif
endif
```
