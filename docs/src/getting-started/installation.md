# Installation & Building

## Arch Linux

If you're on Arch (or an Arch-based distro) with an AUR helper, this is the fastest path — no manual build required:

```bash
yay -S lumen-lang-git
```

Once installed, it's available on your `PATH` as `lumen`. Verify it:

```bash
lumen --version
```

If you're not on Arch, or prefer building from source, follow the steps below instead.

## Building from source

Lumen doesn't have packaged binaries for other platforms yet — you build it from source. This is a five-minute process.

### Requirements

- A Linux or Unix-like operating system
- A C++20-capable compiler
- CMake

### Building

Clone the repository and build with CMake, out-of-source, from the repository root:

```bash
git clone https://github.com/spikest3r/LumenLang.git
cd LumenLang

mkdir build
cd build
cmake ..
make -j$(nproc)
```

Once the build finishes, the executable is available at:

```bash
./build/lumen
```

You can optionally copy or symlink it somewhere on your `PATH` so you can call `lumen` from any directory:

```bash
sudo ln -s "$(pwd)/lumen" /usr/local/bin/lumen
```

(run this from inside the `build` directory, so `$(pwd)` resolves to `.../LumenLang/build`)

## Online

Prefer not to build anything? [Lumen Playground](https://lumen.olehsheremeta.com/playground) runs Lumen entirely in your browser, powered by WebAssembly — no install required. You can open and save both `.lmn` scripts and precompiled `.bin` bytecode files straight from your computer, all client-side. The Playground can also export a compiled program as a scannable QR sequence — see [WebAssembly](../platforms/wasm.md#exporting-programs-as-qr-codes).

## Other targets

The instructions above build the full desktop toolchain (compiler + VM). The VM alone also runs on [Raspberry Pi Pico](../platforms/pico.md) and [AVR](../platforms/avr.md) microcontrollers, and on Android via [LumenRuntimeAndroid](../lumen-in-apps/lumen-on-android.md) — none of these build from this same CMake flow, so see their respective pages under [Platforms & Ports](../platforms/overview.md) for target-specific build steps.

## Verifying the build

Whichever install method you used, check the version and build metadata:

```bash
lumen --version
```

Next: [Your First Program](./first-program.md).