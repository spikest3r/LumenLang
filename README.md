# Interpreter

A small stack-based script compiler and virtual machine written in C++.

### This branch implements VM for Raspberry Pi Pico.

## Overview

This project compiles a simple scripting language into bytecode and executes it with a custom virtual machine. The interpreter supports variables, arithmetic expressions, conditionals, labels and jumps, string output, and integer input.

## Features

- Compile script files into `.bin` bytecode files
- Execute compiled bytecode directly
- Disassemble binary bytecode for debugging
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
./compile
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

- Enable verbose mode:

```bash
./interpreter examples/fizzbuzz.script --verbose
```

- Run in Raspberry Pi Pico mode:

```bash
./interpreter examples/pico/blinky.script --pico
```

## Language syntax

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

### Operators

- Arithmetic: `+`, `-`, `*`, `/`, `%`, `^`
- Comparison: `==`, `!=`, `>`, `<`, `>=`, `<=`

## Built-in functions

- `println <value>` — print a value and newline
- `print <value>` — print a value without newline
- `inputInt &<variable>` — read an integer into a variable

### Raspberry Pi Pico specific functions
- `gpioInit <pin>` — initialize a GPIO pin
- `gpioSetDir <pin> <direction>` — set GPIO direction (`1` for output, `0` for input)
- `gpioPut <pin> <value>` — write a digital value to a pin
- `gpioGet <pin> &<variable>` — read a digital pin into a variable
- `sleepMs <ms>` — pause execution for the given milliseconds
- `gpioPullUp <pin>` — enable pull-up resistor on a pin
- `gpioPullDown <pin>` — enable pull-down resistor on a pin


## Testing

A basic test script is available at `test.sh`.

```bash
./test.sh
```

## Bytecode instruction set

| Opcode | Instruction | Description |
|-------:|-------------|-------------|
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
| `0xFF` | `HLT` | Halt execution |
