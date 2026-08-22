# Input & Output

## Output

| Statement | Description |
|---|---|
| `print <value>` | Print a value without a trailing newline |
| `println <value>` | Print a value followed by a newline |

```lumen
print 'Loading...'
println 'done'
```

## Input

Input statements write into an existing variable, referenced with a leading `&`:

| Statement | Description |
|---|---|
| `inputInt &var` | Read a line from stdin, parse it as an integer, store it in `var` |
| `inputStr &var` | Read a whitespace-delimited token from stdin, store it in `var` |

```lumen
age = 0
print 'Enter your age: '
inputInt &age
println age
```

```lumen
name = ''
print 'Enter your name: '
inputStr &name
println name
```

Every example in this book assigns a placeholder value (`age = 0`, `name = ''`) before reading into a variable with `inputInt`/`inputStr`. This isn't strictly required by the compiler — a variable is registered the first time it's seen, wherever that is — but it's good practice: it documents the variable's intended type up front and avoids relying on whatever default the VM gives an unseen variable.

If `inputInt` receives text that can't be parsed as an integer, it prints `Invalid value!` and leaves the variable at `0`.
