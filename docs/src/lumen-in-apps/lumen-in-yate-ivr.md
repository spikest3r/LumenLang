# Lumen on [Yate IVR](https://github.com/spikest3r/YateIVRcore)

[YateIVRcore](https://github.com/spikest3r/YateIVRcore) embeds Lumen inside an IVR (Interactive Voice Response) core for the [Yate](https://yate.ro) telephony engine. Call-handling logic — menu structure, DTMF handling, call flow — is written entirely as `.lmn` scripts, run by the same VM described in [The Virtual Machine](../architecture/virtual-machine.md), reached from live SIP calls rather than a desktop CLI or a JNI bridge.

This is a different embedding shape than [Android](./lumen-on-android.md), [Pico](../platforms/pico.md), or [WASM](../platforms/wasm.md). Those hosts run one program per invocation, start to finish. The IVR core instead keeps a single Lumen VM instance alive for the duration of one phone call, and repeatedly re-enters it — once when the call starts, once per DTMF digit pressed, once on hangup — while the same globals and state persist across every re-entry.

## Routing calls to scripts

Each Yate extension maps to a `.lmn` script by filename: dialing extension `800` runs `800.lmn`. The core owns the single TCP connection to Yate's `extmodule` interface, parses incoming protocol messages (`call.route`, `chan.dtmf`, `chan.hangup`), and dispatches each one to the matching script — compiling it if this is the first call to reach that extension.

A script only has to define three routines:

- **`onCall`** — runs when the call is answered
- **`onDtmf`** — runs once per DTMF digit pressed during the call
- **`onHangup`** — runs when the call ends

```
routine onCall
    playWav 'C:/VoIP_Assets/welcome_menu.wav'
endroutine

routine onDtmf
    getDtmf &str
    str2int str, &digit

    if digit == 1
        speakToWav 'Hello world', 'C:/VoIP_Assets/temp.wav'
        playWav 'C:/VoIP_Assets/temp.wav'
    else
        if digit == 2
            playWav 'C:/VoIP_Assets/music.wav'
        endif
    endif
endroutine

routine onHangup
    # empty
endroutine
```

Nothing about this script differs syntactically from any other Lumen program — the same [conditionals](../language-guide/conditionals.md), [routines](../language-guide/routines.md), and [variables](../language-guide/variables-and-values.md) covered elsewhere in this book apply unchanged. What's different is entirely on the host side: how and when the VM gets re-entered.

## One VM instance per call

`call.route` creates a fresh VM instance for the call and runs `onCall`. Every subsequent `chan.dtmf` message for that same call re-enters the *same* instance to run `onDtmf`, and `chan.hangup` re-enters it once more for `onHangup` before the instance is torn down. Globals declared at the top of the script — a PIN buffer, a menu state flag, anything — persist across all of these re-entries, because they all execute against the same `ExecutionData`.

This makes call isolation a property of the architecture rather than something a script has to manage. Two people dialing the same extension at the same time get two independent VM instances, each with its own copy of the script's globals; there is no shared state between them to accidentally clobber, and no `call_id`-keyed bookkeeping for a script author to get wrong. The isolation exists whether or not the script author ever thinks about concurrency at all.

## Calling into the VM from the host

The host doesn't run "the program" the way a desktop invocation would. It calls a *specific routine* inside an already-running VM and expects control back once that routine finishes — the call-route handler shouldn't block waiting for the entire call to play out, and the DTMF handler shouldn't restart the program from the top.

This works through the same `RET` mechanism every Lumen routine already uses, without any VM changes: before jumping into a routine, the host pushes a sentinel value onto the routine call stack. The routine runs normally, including any nested `call`s to other routines, and each `RET` pops and compares against the top of that stack. When a `RET` pops the sentinel back off, that's the signal that control has unwound all the way back to the host, not to another Lumen call site — so the host resumes exactly at that point. Nested routine calls made from *inside* `onDtmf` (a shared `playConfirm` routine, say) are unaffected, since they push and pop their own ordinary return addresses beneath the sentinel.

## Native functions

Everything a script uses to affect the outside world — playing audio, synthesizing speech, reading the last DTMF digit, transferring or ending the call — is a native function, dispatched the same way as any [built-in](../reference/builtin-functions.md) or [standard library](../language-guide/standard-library.md) native elsewhere in Lumen.

| Function | Args | Purpose |
|---|---|---|
| `getDtmf` | `&out` | Writes the most recent DTMF digit into `out` |
| `speakToWav` | `text, output_path` | Synthesizes speech to a wav file (blocking) |
| `playWav` | `path` | Plays a wav file on the call |
| `masqueradeTo` | `target` | Masquerades the call onto an arbitrary Yate route |
| `hangUp` | — | Terminates the call |

These sit alongside Lumen's ordinary stdlib natives (`str2int`, `strlen`, `httpRequest`, and so on) — a script mixes both freely, as in the example above.

## Extending the native set without touching the VM

Some functionality is specific to the host machine rather than general enough to belong in the VM core — sending a wake-on-LAN packet, starting a Windows service. Rather than adding these to the VM itself, the IVR core loads them from **extension DLLs** at startup, so the VM and compiler stay unaware of anything host-specific.

An extension DLL exports a single `RegisterNatives` function, which fills in two things: a mapping from opcode to the actual native implementation, using the same function signature every built-in native uses; and a list of descriptors, each naming a function as it should appear in `.lmn` source, together with the opcode it resolves to and how many arguments it expects. The first gives the VM something to dispatch to at runtime; the second gives the compiler what it needs to resolve a call by name at compile time — the same two pieces of information a built-in native carries, just supplied by a DLL loaded at startup instead of compiled into the core.

A DLL missing this export, or one that fails to load, is skipped without affecting the rest of the core. Opcodes are chosen by whoever writes the extension; there's no collision detection yet against the core's own native table or between multiple extensions, so this is worth tracking by hand if more than one extension is in use.

The result is that a `.lmn` script calling a host-specific native like a wake-on-LAN trigger reads no differently from one calling `playWav` — the distinction between "built into the VM" and "loaded from a DLL at startup" is invisible from the script's point of view.
