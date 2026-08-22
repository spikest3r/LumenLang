# Lumen on [Android](https://github.com/spikest3r/LumenRuntimeAndroid)

[LumenRuntimeAndroid](https://github.com/spikest3r/LumenRuntimeAndroid) (`com.spikest3r.lumenruntime`) is a self-contained Lumen toolchain for Android. It bundles an in-app **editor**, the **compiler**, the **VM**, a **disassembler**, and a **QR scanner** for importing programs compiled elsewhere, all behind a Kotlin UI with a JNI bridge into the same C++ core described in [Compiler Pipeline](../architecture/overview.md) and [The Virtual Machine](../architecture/virtual-machine.md).

This places Android in a different category from the [Pico](../platforms/pico.md) and [AVR](../platforms/avr.md) ports. Those are VM-only, compile-on-desktop targets with no on-device compilation path. Android has no such constraint: a `.lmn` program can be written, compiled, disassembled, and run entirely on the device, mirroring the desktop `lumen` CLI's workflow. The QR pipeline is an additional means of getting a program onto the device, not the only one.

## What's on-device

- **Editor.** Write and edit `.lmn` source directly in the app.
- **Compiler.** The same compiler pipeline as desktop, turning `.lmn` source into bytecode (`.bin`) locally — see [Compiler Pipeline](../architecture/overview.md).
- **VM.** The same `execute()` core used on desktop, Pico, and AVR, reached through a JNI bridge.
- **Disassembler.** A live disassembly view alongside the running program, matching the desktop `--disassemble` output described in [Disassembler](../debugging/disassembler.md).
- **QR scanner.** An import path for bytecode compiled elsewhere — covered below.

Because compilation happens on-device, the Android workflow parallels the desktop workflow described in [CLI Reference](../getting-started/cli-reference.md): write source, compile, run, and optionally disassemble, through a touch UI rather than command-line flags.

## The QR import pipeline

The QR path is intended for programs not written on the device itself. Android has no convenient equivalent to dropping a `.bin` file onto a filesystem, so LumenRuntimeAndroid can instead reconstruct a compiled program by scanning a sequence of QR codes.

The pipeline has two sides:

- **Encoding (WASM).** A compiled `.bin` file is split into indexed chunks, each small enough to fit into a single QR code. Every chunk carries its index, the total chunk count, and a CRC32 checksum of its payload.
- **Decoding (Android).** The app scans chunks with the device camera in any order; chunks need not be scanned sequentially. Each incoming chunk is validated against its CRC32 before being accepted, and ingestion is idempotent, so re-scanning an already-captured chunk is a no-op rather than a source of corruption. Once every chunk index from `0` to `total-1` has been seen and validated, the runtime reassembles the original `.bin` bytes and hands them to the same on-device VM used for locally-compiled programs.

This design allows a grid of QR codes for a program to be scanned in whatever order is convenient, rather than requiring a strict sequence.

## Running a program

Once bytecode is ready — whether compiled on-device or reconstructed via QR — the JNI bridge hands it to the native `execute()` loop. Two Android-specific pieces sit around that core:

- **Blocking input.** Lumen's `inputInt`/`inputStr` (see [Input & Output](../language-guide/input-output.md)) expect to block on stdin on desktop. Android has no stdin, so the runtime surfaces an `AlertDialog` instead, with the native thread blocked on a `std::mutex`/`std::condition_variable` pair until the user submits a value through the dialog.
- **Console output.** `print`/`println` calls are relayed from native code to the Kotlin UI thread via a fire-and-forget JNI callback, rendered in a monospace console below the source editor. Output is capped at 1000 lines; once the limit is reached, execution halts rather than continuing to produce output the console can't display.

## Cancellation

Long-running or accidentally-infinite programs — an unbounded `label`/`jump` loop, for instance (see [Labels & Jumps](../language-guide/labels-and-jumps.md)) — are cancellable from the UI. The VM checks a `std::atomic<bool>` cancellation flag cooperatively between instructions, so stopping a program from the Android UI does not require killing the native thread outright; execution unwinds cleanly the next time `execute()` checks the flag.

## Two ways to get a program running

**Written on-device:**

1. Open the editor in LumenRuntimeAndroid and write the `.lmn` source directly.
2. Compile it in-app.
3. Run it, with the disassembler view available alongside.

**Imported from elsewhere:**

1. Write and compile a `.lmn` program on the [WASM Playground](https://lumen.olehsheremeta.com/playground).
2. Export the compiled `.bin` as a QR sequence rather than, or alongside, a regular file download.
3. Open LumenRuntimeAndroid and scan the codes — order does not matter, and the app reports how many chunks remain outstanding.
4. Once reconstruction finishes, run the program directly on-device.

Each example in this book's [Examples](../examples/index.md) chapter includes a QR placeholder alongside its source, generated with this same chunking scheme, so it can be scanned directly into the Android runtime once real codes are filled in — or retyped in the on-device editor.