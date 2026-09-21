# Loops

## while

```lumen
i = 0
while i < 3
  println(i)
  i = i + 1
endwhile
```

The condition is checked before every iteration and follows the rules in [Conditionals](./conditionals.md).

## repeat

```lumen
repeat 5
  println('hi')
endrepeat

repeat 3, i
  println(i)      # 0, 1, 2
endrepeat
```

`repeat <count>` runs its body `count` times. The count must be a single number or variable (an expression such as `n * 2` is a compile error); a variable is re-read on every iteration. With `repeat <count>, <name>` the variable `name` receives the iteration number, starting at 0. After the loop it equals the count.

Loops can nest. Each `repeat` level keeps its own hidden counter.

## break and continue

`break` leaves the innermost loop, `continue` jumps to its next iteration. Both are compile errors outside a loop.

```lumen
i = 0
while i < 10
  i = i + 1
  if i == 3
    continue
  endif
  if i == 6
    break
  endif
  println(i)     # 1 2 4 5
endwhile
```
