# Capabilities

Some standard-library functionality is gated behind **capabilities** — optional features a given VM build may or may not implement (for example, a minimal embedded build might omit `FS` or `HTTP`).

| Function | Description |
|---|---|
| `assertCapability name` | Check whether capability `name` is implemented by this VM build; raises a runtime error if it isn't |

```lumen
assertCapability 'HTTP'
```

Capability names: 
- `'FS'` (file I/O)

- `'random'` (random number generation)

- `'HTTP'` (HTTP requests).

Calling `assertCapability` is optional — the gated functions behave the same whether or not you assert first. It's a guard you add when a script depends on optional functionality and you want a clean, early error on VM builds that don't include it, rather than failing deeper in the program.
