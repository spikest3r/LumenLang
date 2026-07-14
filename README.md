# LumenLang

### WASM Branch

This branch implements LumenLang toolkit for web with WASM and provides assets for [Lumen Playground](https://lumen.olehsheremeta.com/playground).

As of now, not all features are fully implemented.

Currently available features are:

- Compiler
- VM
- Disassembler

---
A lightweight, stack-based scripting language with a custom compiler, bytecode format, and virtual machine written in C++20.

Lumen is designed to be simple to learn while still exposing the concepts behind real programming languages: compilation, bytecode execution, virtual machines, debugging, and optimization.

```text
println 'Hello, world!'

name = ''
print 'What`s your name? '
inputStr &name

greeting = 'Hello, ' .. name .. '!'
println greeting
```

Output:

```text
Hello, world!
What`s your name? Ryan
Hello, Ryan!
```

## Features

### Language

- Variables and dynamic values
- Integer arithmetic
- String manipulation
- String concatenation with `..`
- Conditional execution
- Labels and jumps
- User-defined routines
- Console input/output
- Comments

### Compiler & Virtual Machine

- Compile `.lmn` source files into bytecode
- Execute precompiled bytecode files
- Custom stack-based virtual machine
- Bytecode disassembler
- Debug symbol generation
- Built-in debugger
- Runtime tracing
- Verbose compilation mode

### Developer Experience

- Interactive introduction command
- Built-in example generator
- Beginner-friendly CLI
- Self-contained toolchain

## Getting Started

### Build

Requirements:

- Linux or Unix-like operating system
- C++20 compiler
- CMake
- **Emscripten SDK (emsdk)**

Build from the repository root:

```bash
mkdir build
cd build
emcmake cmake ..
emmake make
```

In build folder you will find `lumen.wasm` and `lumen.js` files.

To run Lumen Playground locally, copy `index.html` from `web` folder to `build` and serve with server.

## Your First Lumen Program

Generate a starter program:

```bash
lumen --introduction
```

Run it:

```bash
lumen helloworld.lmn
```

You can also explore built-in examples:

```bash
lumen --examples
```

Generate an example:

```bash
lumen --examples fizzbuzz
```

Run it:

```bash
lumen fizzbuzz.lmn
```

## CLI Usage

```text
lumen <file> [options]
```

Options:

| Option | Description |
|--------|-------------|
| `--verbose` | Enable verbose compiler output |
| `--compile` | Compile source file |
| `--run` | Execute compiled bytecode |
| `--disassemble` | Show bytecode instructions |
| `--dbgsym` | Generate debug symbols |
| `--debugger` | Run with debugger |
| `--introduction` | Create a starter project |
| `--examples` | List or generate examples |

If no option is provided, Lumen compiles and runs the source file automatically.

## Language Syntax

### Comments

```lumen
# This is a comment

println 'Hello!' # Inline comments work too
```

### Variables

```lumen
number = 42
result = number + 10

println result
```

### Strings

Strings use single quotes:

```lumen
message = 'Hello, Lumen!'
println message
```

Concatenation:

```lumen
name = 'Ryan'
text = 'Hello, ' .. name

println text
```

### Input

Read a string:

```lumen
name = ''

inputStr &name

println name
```

Read an integer:

```lumen
inputInt &age
```

### Conditions

```lumen
if age >= 18
    println 'Adult'
else
    println 'Minor'
endif
```

### Loops with Labels

```lumen
i = 0

label loop

println i

i = i + 1

if i < 10
    jump loop
endif
```

### Routines

```lumen
routine hello

println 'Hello from a routine!'

endroutine

call hello
```

## Built-in Functions

| Function | Description |
|----------|-------------|
| `print` | Print without newline |
| `println` | Print with newline |
| `inputInt &variable` | Read integer input |
| `inputStr &variable` | Read string input |
| `int2str in, &out` | Convert integer to string |
| `str2int in, &out` | Convert string to integer |

## Operators

### Arithmetic

```
+
-
*
/
%
^
```

### Comparison

```
==
!=
>
<
>=
<=
```

## Architecture

Lumen uses a classic compiler pipeline:

```
Source Code (.lmn)
        |
        v
   Lumen Compiler
        |
        v
 Bytecode (.bin)
        |
        v
 Virtual Machine
        |
        v
     Output
```

The VM is stack-based and executes custom bytecode instructions.

## Debugging

Compile with debug symbols:

```bash
lumen program.lmn --dbgsym
```

Run in debugger mode:

```bash
lumen program.lmn --debugger
```

Inspect bytecode:

```bash
lumen program.lmn --disassemble
```

## Examples

Included examples:

| Example | Description |
|---------|-------------|
| `age` | Age calculator |
| `temperature` | Temperature converter |
| `fizzbuzz` | Classic FizzBuzz |
| `infinite-loop` | Labels and jumps demonstration |

Generate:

```bash
lumen --examples fizzbuzz
```

## Testing

Run the test suite:

```bash
./test.sh
```

## Project Goals

Lumen is built to explore:

- How programming languages work
- Compiler design
- Bytecode formats
- Virtual machines
- Debugging systems
- Language tooling

The goal is to keep the language approachable while implementing the same fundamental ideas used by larger language runtimes.

## License

See `LICENSE` for details.