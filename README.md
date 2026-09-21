# LumenLang

A lightweight, stack-based scripting language with a custom compiler, bytecode format, and virtual machine written in C++20.

Lumen is designed to be simple to learn while still exposing the concepts behind real programming languages: compilation, bytecode execution, virtual machines, debugging, and memory management.

```lumen
println('Hello, world!')

print('What`s your name? ')
name = inputStr()

greeting = 'Hello, ' .. name .. '!'
println(greeting)
```

Output:

```text
Hello, world!
What`s your name? Ryan
Hello, Ryan!
```

## Features

### Language

- Variables and dynamic values (integers, floats, strings)
- Integer and floating-point arithmetic
- String concatenation with `..` and escape sequences
- Conditional execution with `if`, `elif`, `else`
- `while` loops and `repeat` loops with `break` / `continue`
- Labels, jumps and `halt`
- Routines and functions with parameters and return values
- Growable arrays
- Script imports
- References and dereferencing
- References and dereferencing
- Console input/output
- Comments

### Standard Library

- File I/O (open, read, write, close)
- Random number generation
- HTTP requests (GET, POST, PUT, DELETE)
- Capability assertions for host-provided features
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
- Stack-based virtual machine
- Managed heap with a mark-and-sweep garbage collector
- Bytecode disassembler
- Inline debug symbols (variables, routines, built-ins, arrays)
- Interactive debugger
- Verbose compilation mode

### Developer Experience

- Starter program generator
- Built-in example generator
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

The file comes first, options after it.

Options:

| Option | Description |
|--------|-------------|
| `--verbose` | Enable verbose compiler and VM output |
| `--compile` | Compile the source file to `<file>.bin` |
| `--run` | Execute compiled bytecode (a `.bin`, or the file just compiled) |
| `--disassemble` | Show the bytecode instructions of a `.bin` |
| `--no-debug` | Do not embed debug symbols in the compiled `.bin` |
| `--debugger` | Run with the interactive debugger |
| `--introduction` | Create a starter program |
| `--examples` | List or generate examples |
| `--version` | Show version, branch, commit, and build date |

If no option is provided, Lumen compiles and runs the source file automatically. Debug symbols are embedded by default.

## Language Syntax

### Comments

```lumen
# This is a comment

println('Hello!') # Inline comments work too
```

### Variables

```lumen
number = 42
result = number + 10

println(result)
```

Numbers can be integers or floats. A literal is treated as a float if it contains a decimal point; otherwise it's an integer:

```lumen
pi = 3.14159
half = 1 / 2

println(pi)
println(half)
```

Mixing an int and a float in an expression promotes the result to a float:

```lumen
total = 10 + 2.5
println(total) # 12.5
```

All variables are global. Reading a variable that was never assigned gives `0`.

### Strings

Strings use single or double quotes:

```lumen
message = 'Hello, Lumen!'
println(message)
```

Concatenation:

```lumen
name = 'Ryan'
text = 'Hello, ' .. name

println(text)
```

Escape sequences `\n`, `\t`, `\r`, `\\`, `\'` and `\"` are supported:

```lumen
println('it\'s a\ttab')
```

### Input

Input functions return the value that was read:

```lumen
name = inputStr()
age = inputInt()

println(name)
println(age)
```

### Conditions

```lumen
if age >= 18
    println('Adult')
elif age >= 13
    println('Teenager')
else
    println('Child')
endif
```

A condition holds one comparison; there is no `and` / `or`, so nest `if` blocks instead. Conditions support full expressions:

```lumen
number = 15

if number % 15 == 0
    println('Divisible by 15')
elif number % 3 == 0
    println('Divisible by 3')
elif number % 5 == 0
    println('Divisible by 5')
else
    println('Not divisible by 3 or 5')
endif
```

### Loops

#### While Loop

```lumen
i = 0

while i < 10
    println(i)
    i = i + 1
endwhile
```

#### Repeat Loop

Repeat a block a fixed number of times. The count is a single number or variable:

```lumen
repeat 5
    println('Hello')
endrepeat
```

With a built-in iterator, the loop variable receives the iteration number, starting at 0:

```lumen
repeat 5, i
    println(i)
endrepeat
```

#### Loop Control

Use `break` to exit a loop early:

```lumen
i = 0

while i < 100
    if i == 5
        break
    endif
    println(i)
    i = i + 1
endwhile
```

Use `continue` to skip to the next iteration:

```lumen
repeat 10, i
    if i % 2 == 0
        continue
    endif
    println(i)
endrepeat
```

#### Labels and Jumps

```lumen
i = 0

label loop

println(i)

i = i + 1

if i < 10
    jump loop
endif
```

`halt` stops the program immediately.

### Routines and Functions

A routine performs actions; a function also returns a value:

```lumen
routine greet(name, times)
    repeat times
        println('Hello, ' .. name)
    endrepeat
endroutine

function add(a, b)
    return a + b
endfunction

greet('Ann', 2)
total = add(2, 3)
println(total)
```

Parameters are ordinary global variables assigned on entry; there is no local scope.

### Arrays

Arrays are named, untyped and grow on demand. Writing to an index creates the array:

```lumen
scores[0] = 10
scores[1] = 20
scores[2] = scores[0] + scores[1]

println(scores[2]) # 30
```

Reading a missing array or an index out of range is a runtime error.

### Imports

```lumen
import 'lib.lmn'
```

The imported script is compiled into the program at that point. Its routines, functions, variables and labels share the global scope.

### References and Dereferencing

```lumen
value = 20

ref = &value
value = 30

println(*ref) # 30
```

## Built-in Functions

Functions that return a value are used in expressions, for example `n = str2int('42')`.

### Core

| Function | Description |
|----------|-------------|
| `print(value)` | Print without newline |
| `println(value)` | Print with newline |
| `inputInt()` | Read an integer, returns it (`0` if invalid) |
| `inputStr()` | Read one word, returns it |
| `int2str(n)` | Convert integer to string |
| `str2int(s)` | Convert string to integer (runtime error if invalid) |
| `float2str(x)` | Convert number to string |
| `str2float(s)` | Convert string to float (`0` if invalid) |

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

A script can declare the host features it needs. If the host does not provide the capability, the program stops with an error.

| Function | Description |
|----------|-------------|
| `assertCapability(name)` | Assert that a capability (e.g. `'FS'`, `'random'`, `'HTTP'`) is available |

### File I/O

| Function | Description |
|----------|-------------|
| `openFile(path)` | Open a file for reading and writing (created if missing, contents kept), returns a handle |
| `writeFile(data, handle)` | Append a string to an open file |
| `readFile(handle)` | Return the full contents of an open file |
| `closeFile(handle)` | Close an open file handle |

```lumen
h = openFile('log.txt')
writeFile('hello\n', h)
println(readFile(h))
closeFile(h)
```

### Random

| Function | Description |
|----------|-------------|
| `randomSeed(seed)` | Seed the random number generator |
| `random()` | Random float in `[0.0, 1.0)` |
| `randomRange(min, max)` | Random integer in `[min, max]` (inclusive) |

### HTTP

| Function | Description |
|----------|-------------|
| `httpRequest(method, url, headers, body, &response)` | Perform an HTTP request, returns the status code |

- `method` — `'GET'`, `'POST'`, `'PUT'`, or `'DELETE'` (case-insensitive)
- `url` — full URL including scheme, e.g. `'http://example.com/path'`
- `headers` — newline-separated `Key: Value` pairs, or `''` for none
- `body` — request body string, ignored for `GET`
- `&response` — reference to the variable that receives the response body, or an error message on failure
- return value — the HTTP status code, or `-1` on a connection-level failure

```lumen
assertCapability('HTTP')

body = ''
status = httpRequest('GET', 'http://example.com/', '', '', &body)

println('Status: ' .. status)
println(body)
```

```lumen
assertCapability('HTTP')

body = ''
status = httpRequest('POST', 'http://httpbin.org/post', 'Content-Type: application/json', '{"key":"value"}', &body)
println(body)
```

> HTTPS URLs are not currently supported — only plain `http://` requests.

### String Manipulation

| Function | Description |
|----------|-------------|
| `strlen(s)` | Length of a string |
| `substr(s, start, len)` | Extract a substring |
| `strfind(s, needle)` | Index of the first occurrence of `needle` in `s`, or `-1` if not found |
| `strcase(s, upper)` | Convert case: `upper = 1` for uppercase, `0` for lowercase |
| `trim(s)` | Strip leading and trailing whitespace |

```lumen
println('Length: ' .. strlen('hello world'))
println('Substring: ' .. substr('hello world', 6, 5))
println('Found at: ' .. strfind('hello world', 'world'))
println(strcase('Hello World', 1))
println('[' .. trim('   padded   ') .. ']')
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

Arithmetic works across ints and floats. `/` always produces a float; the other operators return a float if either operand is a float, and an int otherwise. Arithmetic on a string is a runtime error.

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

The VM is stack-based and executes custom bytecode instructions. Variables and arrays live in a managed heap that is reclaimed by a mark-and-sweep garbage collector. Each program can use at most 256 variables, 256 arrays, 256 string literals and 256 numeric constants.

## Debugging

Compiled programs carry debug symbols by default. Skip them with:

```bash
lumen program.lmn --compile --no-debug
```

Run in debugger mode:

```bash
lumen program.lmn --debugger
```

Inspect bytecode:

```bash
lumen program.lmn --compile
lumen program.lmn.bin --disassemble
```

Inside the debugger, `variables` lists variables and arrays with their names and contents.

## Documentation

The full guide lives in `docs/` and is built with [mdBook](https://rust-lang.github.io/mdBook/):

```bash
cd docs
mdbook build
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
- Virtual machines and memory management
- Debugging systems
- Language tooling

The goal is to keep the language approachable while implementing the same fundamental ideas used by larger language runtimes.

## License

See `LICENSE` for details.