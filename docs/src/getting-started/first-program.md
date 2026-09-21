# Your First Program

Let the interpreter write a starter script for you:

```bash
lumen --introduction
```

This creates `helloworld.lmn` in the current directory:

```lumen
println('Hello, world!')

print('What`s your name? ')
name = inputStr()

greeting = 'Hello, ' .. name .. '!'
println(greeting)
```

Run it:

```bash
lumen helloworld.lmn
```

```text
Hello, world!
What`s your name? Ryan
Hello, Ryan!
```

## What happened

1. `lumen helloworld.lmn` compiled the script to `helloworld.lmn.bin` and immediately ran it. See [CLI Reference](./cli-reference.md) to do the two steps separately.
2. `println('...')` and `print('...')` are built-in functions. `println` appends a newline, `print` does not.
3. `inputStr()` reads one whitespace-delimited word from standard input and *returns* it, so it is used on the right-hand side of an assignment.
4. `..` joins strings. Numbers are converted automatically.

The backtick in ``What`s`` is only a habit: `\'` also works inside a single-quoted string. See [Strings](../language-guide/strings.md).

## Next steps

- Generate the bundled programs with `lumen --examples <name>` (see [Examples](../examples/index.md)).
- Learn the [language](../language-guide/comments.md) topic by topic.
- Step through the program with `lumen helloworld.lmn --debugger`, see [Interactive Debugger](../debugging/interactive-debugger.md).
