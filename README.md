# Interpreter

A small stack-based script compiler and virtual machine written in C++.

## Overview

This project compiles a simple scripting language into bytecode and executes it with a custom virtual machine. The interpreter supports variables, arithmetic expressions, conditionals, labels and jumps, string output, and integer input.

## Features

- Compile script files into `.bin` bytecode files
- Execute compiled bytecode directly
- Disassemble binary bytecode for debugging
- Built-in debugger with breakpoint, and stack/variable trace,
- Built-in functions: `println`, `print`, `inputInt`
- Arithmetic operators: `+`, `-`, `*`, `/`, `%`, `^`
- Comparison operators: `==`, `!=`, `>`, `<`, `>=`, `<=`
- Control flow with `if`, `else`, `endif`, `label`, and `jump`

## Requirements

- Linux or Unix-like OS
- `g++` with C++20 support

## Build

From the repository root:

```bash
mkdir build
cd build
cmake ..
make -j($nproc)
```

## Usage

```bash
./interpreter <script-file> [--verbose] [--compile] [--run] [--disassemble]
```

Examples:

- Compile and run a script:

```bash
./interpreter examples/fizzbuzz.script
```

- Compile only:

```bash
./interpreter examples/fizzbuzz.script --compile
```

- Run compiled bytecode only:

```bash
./interpreter examples/fizzbuzz.script.bin --run
```

- Disassemble a compiled binary:

```bash
./interpreter examples/fizzbuzz.script.bin --disassemble
```

- Run script with debugger

```bash
./interpreter examples/fizzbuzz.script --debugger
```

- Run precompiled binary with debugger

```bash
./interpreter examples/fizzbuzz.script.bin --run --debugger
```

- Enable verbose mode:

```bash
./interpreter examples/fizzbuzz.script --verbose
```

## Language syntax

### Comments

Only one-line comments are available for now.

```
# One-line comment
println 'Hello, world!' # This is a comment
```

### Variables

Assign values to variables with `=`:

```text
a = 10
b = a + 2
```

### Strings

String literals use single quotes:

```text
print 'Hello, world'
println 'done'
```

Concatenate stings:

```text
mystr = 'He' .. 'llo'
result = mystr .. ' world'
println result
```

### Input

Read an integer from stdin into a variable:

```text
inputInt &N
```

### Conditionals

```text
if a == b
    println 'equal'
else
    println 'not equal'
endif
```

### Labels and jumps

```text
label start
    i = i + 1
    if i <= 10
        jump start
    endif
```

### Subroutines

```text
routines sayhello
println 'Hello, world!'
endroutine

println 'My routine'
call sayhello
```

### Operators

- Arithmetic: `+`, `-`, `*`, `/`, `%`, `^`
- Comparison: `==`, `!=`, `>`, `<`, `>=`, `<=`

## Built-in functions

- `println <value>` — print a value and newline
- `print <value>` — print a value without newline
- `inputInt &<variable>` — read an integer into a variable

## Testing

A basic test script is available at `test.sh`.

```bash
./test.sh
```

## Bytecode instruction set

| Opcode | Instruction | Description |
|-------:|-------------|-------------|
| `0x01` | `CALL` | Call user-defined subroutine |
| `0x02` | `POP` | Pop top of stack into variable |
| `0x03` | `PUSH` | Push literal, variable or string to stack |
| `0x04` | `EXEC` | Call built-in function |
| `0x05` | `JUMP` | Jump to bytecode address |
| `0xA0` | `ADD` | Add two integers |
| `0xA1` | `SUB` | Subtract two integers from stack|
| `0xA2` | `MUL` | Multiply two integers from stack|
| `0xA3` | `DIV` | Divide two integers from stack|
| `0xA4` | `POW` | Power operator from stack|
| `0xA5` | `MOD` | Modulo operator from stack|
| `0xB0` | `JEQ` | Jump if equal |
| `0xB1` | `JGR` | Jump if greater |
| `0xB2` | `JLS` | Jump if less |
| `0xB3` | `JGE` | Jump if greater or equal |
| `0xB4` | `JLE` | Jump if less or equal |
| `0xB5` | `JNE` | Jump if not equal |
| `0xAA` | `JOIN` | Concatenate strings from stack |
| `0xFF` | `HLT` | Halt execution |
