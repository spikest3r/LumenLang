# LumenLang

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
- Integer and floating-point arithmetic
- String manipulation
- String concatenation with `..`
- Conditional execution
- Labels and jumps
- User-defined routines
- References and dereferencing
- References and dereferencing
- Console input/output
- Comments

### Standard Library

- File I/O (open, read, write, close)
- Random number generation
- HTTP requests (GET, POST, PUT, DELETE)
- Capability-gated access to sensitive builtins
- String inspection and manipulation

### Standard Library

- File I/O (open, read, write, close)
- Random number generation
- HTTP requests (GET, POST, PUT, DELETE)
- Capability-gated access to sensitive builtins
- String inspection and manipulation

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

Build from the repository root:

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

The executable will be available as:

```bash
./lumen
```

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
| `--version` | Show version, branch, commit, and build date |

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

Numbers can be integers or floats. A literal is treated as a float if it contains a decimal point or an exponent (`3.14`, `1e-2`); otherwise it's an integer:

```lumen
pi = 3.14159
half = 1 / 2

println pi
println half
```

Mixing an int and a float in an expression promotes the result to a float:

```lumen
total = 10 + 2.5
println total # 12.5
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

### References and Dereferencing

```lumen
value = 20

ref = &value

deref = *ref
```

## Built-in Functions

### Core

| Function | Description |
|----------|-------------|
| `print value` | Print without newline |
| `println value` | Print with newline |
| `inputInt &out` | Read integer input |
| `inputStr &out` | Read string input |
| `int2str in, &out` | Convert integer to string |
| `str2int in, &out` | Convert string to integer |
| `float2str in, &out` | Convert float to string |
| `str2float in, &out` | Convert string to float |

### Pico native functions

| Function | Description |
|----------|-------------|
| `gpioInit pin` | Initialize a GPIO pin as input (clears DDR bit) |
| `gpioSetDir pin, mode` | Set GPIO pin direction (1 = output, 0 = input) |
| `gpioPut pin, value` | Write a digital value (high/low) to a GPIO pin |
| `gpioGet pin, &var` | Read a GPIO pin's input state into a variable |
| `gpioPullUp pin` | Enable the internal pull-up resistor on a pin |
| `gpioPullDown pin` | Enable pull-down on a pin (unsupported on this platform; hangs) |
| `sleepMs ms` | Busy-wait sleep for the given number of milliseconds |

### Capabilities

Some builtins require a capability to be asserted before use. Attempting to use a gated feature without asserting its capability first raises a runtime error.

| Function | Description |
|----------|-------------|
| `assertCapability name` | Assert that a capability (e.g. `'FS'`, `'random'`, `'HTTP'`) is available |

### File I/O

Requires the `FS` capability.

| Function | Description |
|----------|-------------|
| `openFile path, &handle` | Open a file, returns a handle |
| `writeFile data, handle` | Write a string to an open file |
| `readFile &out, handle` | Read the full contents of an open file |
| `closeFile handle` | Close an open file handle |

### Random

Requires the `random` capability.

| Function | Description |
|----------|-------------|
| `randomSeed seed` | Seed the random number generator |
| `random &out` | Generate a random float in `[0.0, 1.0)` |
| `randomRange min, max, &out` | Generate a random integer in `[min, max]` (inclusive) |

### HTTP

Requires the `HTTP` capability.

| Function | Description |
|----------|-------------|
| `httpRequest method, url, headers, body, &status, &response` | Perform an HTTP request |

- `method` — `'GET'`, `'POST'`, `'PUT'`, or `'DELETE'` (case-insensitive)
- `url` — full URL including scheme, e.g. `'http://example.com/path'`
- `headers` — newline-separated `Key: Value` pairs, or `''` for none
- `body` — request body string, ignored for `GET`
- `status` — receives the HTTP status code, or `-1` on a connection-level failure
- `response` — receives the response body, or an error message on failure

```lumen
assertCapability 'HTTP'

httpRequest 'GET', 'http://example.com/', '', '', &status, &body

int2str status, &statusStr
println 'Status: ' .. statusStr
println body
```

```lumen
assertCapability 'HTTP'

httpRequest 'POST', 'http://httpbin.org/post', 'Content-Type: application/json', '{"key":"value"}', &status, &body
println body
```

> HTTPS URLs are not currently supported — only plain `http://` requests.

### String Manipulation

| Function | Description |
|----------|-------------|
| `strlen s, &out` | Length of a string |
| `substr s, start, len, &out` | Extract a substring |
| `strfind s, needle, &out` | Index of the first occurrence of `needle` in `s`, or `-1` if not found |
| `strcase s, upper, &out` | Convert case: `upper = 1` for uppercase, `0` for lowercase |
| `trim s, &out` | Strip leading and trailing whitespace |

```lumen
strlen 'hello world', &len
int2str len, &lenStr
println 'Length: ' .. lenStr

substr 'hello world', 6, 5, &sub
println 'Substring: ' .. sub

strfind 'hello world', 'world', &idx
int2str idx, &idxStr
println 'Found at: ' .. idxStr

strcase 'Hello World', 1, &upper
println upper

trim '   padded   ', &trimmed
println '[' .. trimmed .. ']'
```

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

Arithmetic works across ints and floats. `/` always produces a float; the other operators return a float if either operand is a float, and an int otherwise.

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