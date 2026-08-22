# Your First Lumen Program

The fastest way to get oriented is to let Lumen generate a starter script for you.

## Generate a starter program

```bash
lumen --introduction
```

This writes a file called `helloworld.lmn` in the current directory:

```lumen
println 'Hello, world!'

name = ''
print 'What`s your name? '
inputStr &name

greeting = 'Hello, ' .. name .. '!'
println greeting
```

It also prints a short welcome message pointing you at the next steps.

## Run it

```bash
lumen helloworld.lmn
```

```
Hello, world!
What`s your name? Ryan
Hello, Ryan!
```

When you run a `.lmn` file with no flags, Lumen **compiles and executes it in one step** — you don't need to invoke the compiler and VM separately unless you want to (see [CLI Reference](./cli-reference.md)).

## Explore the built-in examples

Lumen ships with a handful of example programs baked into the binary. List them:

```bash
lumen --examples
```

```
Available examples:
  age               Age calculator
  infinite-loop     Infinite loop demonstrating labels and jumps
  temperature       Temperature converter
  fizzbuzz          Classical FizzBuzz algorithm

Generate an example:
  lumen --examples <name>
```

Generate one to disk:

```bash
lumen --examples fizzbuzz
```

```
Created 'fizzbuzz.lmn'!
Run it with: lumen fizzbuzz.lmn
```

And run it:

```bash
lumen fizzbuzz.lmn
```

Each of these is walked through in detail in the [Examples](../examples/index.md) chapter.

## What's next

- Read through the [Language Guide](../language-guide/comments.md) to learn Lumen's syntax feature by feature.
- Or jump straight to the [CLI Reference](./cli-reference.md) to see every flag `lumen` supports.
