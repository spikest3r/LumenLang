# Lumen

Lumen is a small, embeddable scripting language. Source files (`.lmn`) are compiled to compact bytecode (`.bin`) that a tiny stack-based virtual machine executes. The same VM runs on the desktop, in the browser (WebAssembly), on microcontrollers and inside host applications.

```lumen
println('Hello, world!')

print('What`s your name? ')
name = inputStr()

greeting = 'Hello, ' .. name .. '!'
println(greeting)
```

## What the language looks like

- **Line-oriented.** One statement per line, no semicolons, block statements closed with `endif`, `endwhile`, `endrepeat`, `endroutine`, `endfunction`.
- **Dynamically typed.** Values are integers, floats or strings, and a variable can change type on every assignment. No declarations.
- **Function-call syntax.** Built-ins, routines and functions are called with parentheses: `println('x')`, `n = str2int('42')`.
- **Global state.** All variables are global; routines and functions share them.
- **Arrays.** Named, growable, untyped arrays: `scores[0] = 10`.
- **Modular.** `import 'file.lmn'` pulls another script into the program.
- **Garbage-collected memory.** Variable and array storage lives in a managed heap with a mark-and-sweep collector.

## How this book is organised

- **Getting Started** covers building the interpreter, writing a first program and the command line.
- **Language Guide** documents every statement and operator.
- **Examples** are complete programs you can generate with `lumen --examples <name>`.
- **Architecture**, **Debugging & Tooling** and **Reference** describe the compiler, the bytecode, the VM, the debugger and every opcode and built-in function.
- **Lumen in apps** and **Platforms & Ports** describe projects that embed or port the VM. They follow their own release schedule and may lag behind the language described in the Language Guide.

This book describes Lumen v4 (`lumen --version`).
