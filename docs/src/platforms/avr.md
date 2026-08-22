# AVR

Alongside the Pico port, Lumen's VM also runs bare-metal on AVR microcontrollers (the classic Arduino-family chips). Like Pico, this is a VM-only port — compilation stays on desktop, and only the compiled `.bin` bytecode makes the trip to the chip. Unlike Pico, the AVR build is compiled and flashed through the **Arduino IDE**, not a cross-compiling CMake toolchain.

## Why AVR alongside Pico

Pico (a Cortex-M0+, 32-bit, with a real amount of RAM by microcontroller standards) and AVR (8-bit, far tighter on RAM and flash) sit at very different points on the embedded spectrum. Targeting both is a useful stress test for the VM's portability assumptions: if the bytecode interpreter's core loop and `Variant` value representation can run correctly on an 8-bit AVR with a few kilobytes of RAM, that's a strong signal the VM itself doesn't secretly depend on 32-bit-friendly assumptions anywhere in its hot path.

## What's shared with Pico

The AVR build follows the same structural split described in [Raspberry Pi Pico](./pico.md):

- The same `execute()` fetch-decode-execute loop, unchanged in logic from desktop.
- `funcTable` (a plain array of `NativeFn` pointers) instead of desktop's `funcMap`, for the same reasons — no heavier STL containers on constrained hardware.
- The same `0xD0`–`0xFF` reserved `EXEC` range for host-specific native functions, so a bytecode file that calls into that range means something different (and needs a different `funcTable`) depending on which board it's destined for.

## What's different

AVR's tighter memory budget means more care is needed around bytecode size and the constant/string pools (see [Bytecode Format](../architecture/bytecode-format.md)) than on Pico or desktop — a `.lmn` program with a large number of distinct string or float literals costs proportionally more on an AVR target than the same program would on Pico or desktop, simply because there's less RAM to hold the deduplicated pool in. In practice this mostly affects program *design* rather than the VM itself: keeping AVR-bound programs lean on literals goes a long way.

The build process is also different in kind, not just in tooling — see below.

## Getting bytecode onto the device

The AVR VM lives in the `avr-vm` project as an Arduino sketch, not a standalone CMake build. Getting a program running on-device is a manual, multi-step process:

1. Compile the `.lmn` program on desktop as usual (`lumen program.lmn --compile`), producing a `.bin` file.
2. Run `bin2h` (found alongside the sketch in `avr-vm`) against that `.bin` file. It converts the compiled bytecode into a C header — a byte array suitable for `#include`.
3. Paste the generated header's contents into `program.h` inside the Arduino sketch.
4. Open the sketch in the **Arduino IDE**, having first imported all the sources and headers from `avr-vm`'s `src` and `inc` directories into the sketch.
5. Build and upload the sketch to the board from the IDE, the same way as any other Arduino project.

There's no dynamic loading step — the bytecode is baked into `program.h` at build time, so a new program means regenerating the header with `bin2h` and re-uploading the sketch, not flashing a separate data blob alongside the firmware.