# Platforms & Ports

The core compiler and VM described in [Architecture](../architecture/overview.md) target desktop Linux, but the same bytecode format is designed to run identically everywhere. Lumen currently spans four targets:

| Target | What runs | Notes |
|---|---|---|
| **Desktop** (Linux/Arch) | Full toolchain — compiler, VM, debugger, disassembler | The `lumen` CLI described throughout this book |
| **[Raspberry Pi Pico](./pico.md)** | VM only, bare-metal C | No compiler on-device — bytecode is cross-compiled on desktop and flashed/loaded onto the Pico |
| **[AVR](./avr.md)** | VM only, bare-metal C | Same split as Pico: desktop compiles, AVR executes |
| **[WebAssembly](./wasm.md)** | Full toolchain, in-browser | Powers the [Lumen Playground](https://lumen.olehsheremeta.com/playground); can also export compiled programs as QR codes |
| **[Android](../lumen-in-apps/lumen-on-android.md)** | Full toolchain — editor, compiler, VM, disassembler, JNI-wrapped | Also has a QR scanner for importing programs compiled elsewhere; not the only way to get a program onto the device |

## Why Pico and AVR ship the VM without the compiler

Pico and AVR ship the VM (`execute()` and its surrounding loop, see [The Virtual Machine](../architecture/virtual-machine.md)) but not the compiler. This is a deliberate split, specific to those two microcontroller targets:

- The compiler depends on the C++ standard library's string handling and file I/O in ways that either don't exist or aren't worth porting to a microcontroller.
- Compiled bytecode (a `.bin` file, see [Bytecode Format](../architecture/bytecode-format.md)) is a small, flat, self-describing blob — trivial to embed as a C byte array or flash over a wire.
- Keeping compilation on desktop means the Pico and AVR VMs only have to get `execute()` right, not an entire tokenizer/parser/codegen pipeline.

So the workflow on Pico and AVR is the same shape: **compile on desktop, transfer the `.bin`, run the VM on-device**, as a flashed C byte array in both cases.

Android and WebAssembly don't follow this split — both ship the full compiler alongside the VM, so a program can be written and compiled without ever leaving the device or browser. Android's QR scanner is a convenience for importing bytecode compiled elsewhere, not a substitute for an on-device compiler it lacks.

## Native function tables per target

[Built-in Functions](../reference/builtin-functions.md) covers this in more detail, but the short version: desktop dispatches `EXEC` through `funcMap`, a table of C++ lambdas. Embedded targets (Pico, AVR) use `funcTable`, a plain array of `NativeFn` function pointers, with indices `0xD0`–`0xFF` reserved for host-specific natives like GPIO calls. This split lets each platform expose exactly the native functions that make sense for it — printing and input on desktop, pin control on a microcontroller — without changing the VM's core dispatch loop.