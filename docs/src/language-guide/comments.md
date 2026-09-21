# Comments

A `#` starts a comment that runs to the end of the line. It can follow code, and a `#` inside a string literal is just text.

```lumen
# a full-line comment
x = 5 # a trailing comment
println('a # not a comment')
```

There are no block comments. Indentation and blank lines are ignored, so indent bodies however you like.
