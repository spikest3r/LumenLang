# WebAssembly

Unlike the Pico and AVR ports, the WebAssembly build carries the **full toolchain** — compiler and VM both — compiled to WASM and running entirely client-side in the browser. This is what powers [Lumen Playground](https://lumen.olehsheremeta.com/playground): there's no server-side execution, so programs you write in the Playground never leave your machine unless you choose to export them.

## What runs in-browser

- The same compiler pipeline described in [Compiler Pipeline](../architecture/overview.md), turning `.lmn` source into bytecode.
- The same VM described in [The Virtual Machine](../architecture/virtual-machine.md), executing that bytecode.
- File open/save for both `.lmn` source and precompiled `.bin` bytecode, backed by the browser's native file picker rather than a real filesystem.

Because it's the full toolchain rather than a VM-only port, the WASM build doesn't need the `funcTable`/`0xD0`–`0xFF` split that Pico and AVR use — it dispatches built-ins the same way desktop does.

## Exporting programs as QR codes

The Playground can take a compiled `.bin` program and export it as a sequence of QR codes instead of (or alongside) a downloadable file. This exists specifically to bridge to targets that don't have an easy file-transfer path of their own — most notably [Lumen on Android](../lumen-in-apps/lumen-on-android.md), which imports programs by scanning a QR sequence rather than pulling a file off a filesystem.

The underlying chunking scheme is target-agnostic: a `.bin` file is split into indexed chunks small enough to fit in a single QR code, each carrying enough information (chunk index, total count, a CRC32 checksum) to be reconstructed in any scan order and validated for corruption. The same scheme has a desktop-side implementation too — see [Lumen on Android](../lumen-in-apps/lumen-on-android.md#the-qr-import-pipeline) for how the pieces fit together end to end.

## Why this matters for the embedded/mobile split

Between the four non-desktop targets, WASM is really the odd one out in a useful way: it's the only port that both compiles *and* is reachable from a phone camera. That combination is what makes it the natural bridge for getting a Lumen program from "written on a laptop" to "running on Android" without ever touching a cable, a file share, or a server.
