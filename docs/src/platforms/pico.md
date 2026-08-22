# Raspberry Pi Pico

Lumen's VM runs bare-metal on the Raspberry Pi Pico, built against [pico-sdk](https://github.com/raspberrypi/pico-sdk) with `devkitPro`/CMake toolchains. There's no compiler on-device — you write and compile `.lmn` programs on desktop, then get the resulting `.bin` bytecode onto the Pico.

## What's ported

The Pico build reuses the same `execute()` loop as desktop (see [The Virtual Machine](../architecture/virtual-machine.md)), with two structural differences:

- **`funcTable` instead of `funcMap`.** Desktop's native function table is a `std::map`-backed set of lambdas; the Pico build uses a plain array of `NativeFn` function pointers, since embedded C++ toolchains make heavier STL containers more expensive than they're worth here.
- **A GPIO-specific `EXEC` range.** Function indices `0xD0`–`0xFF` are reserved on Pico for application-specific native functions — GPIO writes, I2C calls, and similar host behavior — mapped onto the tail end of `funcTable` at a fixed offset from the base opcode. Desktop programs don't use this range at all; it only means something once bytecode is destined for a Pico.

## Getting bytecode onto the device

1. Write and compile your `.lmn` program on desktop as usual (`lumen program.lmn --compile`), producing a `.bin` file — see [CLI Reference](../getting-started/cli-reference.md).
2. Cross-compile the Pico firmware image, embedding the `.bin` bytecode as a C byte array baked into the binary (rather than reading from a filesystem, which the Pico doesn't have in the general case).
3. Flash the resulting `.uf2` image onto the Pico over USB in bootloader mode, the same way you'd flash any other pico-sdk project.

## Peripheral access

Programs that need to touch hardware — an I2C display, a GPIO pin, a UART line — do so through native functions registered in the `0xD0`–`0xFF` range, exactly like any other built-in from the language's point of view: a compiled `EXEC` call with a function index. From inside a `.lmn` program, a GPIO write looks like any other built-in call; the fact that it's toggling a physical pin rather than printing to a terminal is entirely a property of what's registered in `funcTable` on that build, not something the language syntax needs to know about.