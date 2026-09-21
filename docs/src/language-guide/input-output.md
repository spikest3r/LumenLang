# Input & Output

## Output

```lumen
print('no newline')
println('with newline')
println(42)
println('x = ' .. x)
```

`print` and `println` take exactly one argument, any expression. Floats print with up to six significant digits.

## Input

Input functions *return* the value read:

```lumen
age = inputInt()
name = inputStr()
```

- `inputInt()` reads a whitespace-delimited token and converts it to an integer. If it is not a valid integer it prints `Invalid value!` and returns `0`.
- `inputStr()` reads one whitespace-delimited word. It cannot read a line containing spaces.

Both read from standard input, so the prompt has to be printed first:

```lumen
print('Enter your birth year: ')
year = inputInt()
```
