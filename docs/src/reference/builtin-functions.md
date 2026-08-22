# Built-in Functions

Built-in functions aren't opcodes — they're entries in a native function table (`funcMap`, `src/vmfuncmap.cpp`) invoked through the single `EXEC` (`0x04`) opcode, which looks up a function index and calls the matching C++ lambda with the current stack and variable storage. The compiler maps each keyword to the same index via `funcList` (`src/compiler.cpp`).

| Keyword | Function index | Signature | Behavior |
|---|---|---|---|
| `println` | `0x01` | `println <value>` | Pop the stack top, print it, then print a newline |
| `print` | `0x02` | `print <value>` | Pop the stack top, print it, no trailing newline |
| `inputInt` | `0x03` | `inputInt &var` | Read a token from stdin, parse as `int64`, store into the referenced variable slot as `TAG_INT`. On parse failure, prints `Invalid value!` and leaves the variable's value untouched (whatever it held before, which defaults to `0`) |
| `inputStr` | `0x04` | `inputStr &var` | Read a whitespace-delimited token from stdin, store into the referenced variable slot as `TAG_STRING` |
| `str2int` | `0x05` | `str2int <value> &out` | Pop the referenced variable slot (destination), then the value (`TAG_STRING` expected; anything else is treated as `"0"`), parse as `int32`, store into the destination as `TAG_INT`. On parse failure or out-of-range, stores `0` instead |
| `int2str` | `0x06` | `int2str <value> &out` | Pop the referenced variable slot (destination), then the value (only `TAG_INT` is read; other types fall back to `0`), format as a string, store into the destination as `TAG_STRING` |
| `str2float` | `0x07` | `str2float <value> &out` | Pop the referenced variable slot (destination), then the value (`TAG_STRING` expected; anything else is treated as `"0"`), parse as `double`, store into the destination as `TAG_FLOAT`. On parse failure or out-of-range, stores `0.0` instead |
| `float2str` | `0x08` | `float2str <value> &out` | Pop the referenced variable slot (destination), then the value — accepts both `TAG_FLOAT` and `TAG_INT` (integers are widened to `double`), format as a string via `std::to_string`, store into the destination as `TAG_STRING` |
| `strlen` | — | `strlen s, &out` | Length of `s`, stored into `out` as `TAG_INT` |
| `substr` | — | `substr s, start, len, &out` | Extract a substring of `s` starting at `start` with length `len`, stored into `out` as `TAG_STRING` |
| `strfind` | — | `strfind s, needle, &out` | Index of the first occurrence of `needle` in `s`, or `-1` if not found, stored into `out` |
| `strcase` | — | `strcase s, upper, &out` | Convert case of `s`: `upper = 1` for uppercase, `0` for lowercase, stored into `out` |
| `trim` | — | `trim s, &out` | Strip leading and trailing whitespace from `s`, stored into `out` |
| `assertCapability` | — | `assertCapability name` | Check whether capability `name` (`'FS'`, `'random'`, `'HTTP'`) is implemented by this VM build; raises a runtime error if not. Optional — gated functions work whether or not it's called first |
| `openFile` | — | `openFile path, &handle` | Open a file at `path`, storing a handle in `handle`. Gated by the `FS` capability |
| `writeFile` | — | `writeFile data, handle` | Write string `data` to the file at `handle`. Gated by the `FS` capability |
| `readFile` | — | `readFile &out, handle` | Read the full contents of the file at `handle` into `out`. Gated by the `FS` capability |
| `closeFile` | — | `closeFile handle` | Close the file at `handle`. Gated by the `FS` capability |
| `randomSeed` | — | `randomSeed seed` | Seed the random number generator. Gated by the `random` capability |
| `random` | — | `random &out` | Generate a random float in `[0.0, 1.0)` into `out`. Gated by the `random` capability |
| `randomRange` | — | `randomRange min, max, &out` | Generate a random integer in `[min, max]` into `out`. Gated by the `random` capability |
| `httpRequest` | — | `httpRequest method, url, headers, body, &status, &response` | Perform an HTTP request (`GET`/`POST`/`PUT`/`DELETE`); `status` receives the HTTP status code (`-1` on connection failure), `response` receives the body. HTTPS is not supported. Gated by the `HTTP` capability |

The eight core functions above still occupy indices `0x01`–`0x08`. Gated standard-library functions check capability support on the same `EXEC` dispatch mechanism; see [Capabilities](../language-guide/capabilities.md).

Note the argument order: for the two-argument conversion functions, tokens are pushed onto the stack in the order they're written, so the *last*-written token is popped *first*. Since the destination `&out` is written last, it's popped first as the destination slot, then the value is popped second — hence `str2int <value> &out`, not `str2int &out <value>`.

## Adding a new built-in function

Since dispatch goes through a single opcode and a lookup table, adding a new built-in doesn't require touching the opcode set or the VM's core loop:

1. Add an entry to `funcMap` in `src/vmfuncmap.cpp` — a lambda taking `(stack, variables)`, with whatever native behavior you want.
2. Add a matching entry to `funcList` in `src/compiler.cpp` so the compiler recognizes the keyword and knows what index to emit.
3. Pick an unused function index — `0x01`–`0x08` are taken by the eight built-ins above.

That's it; no changes to `execute()` in `src/vm.cpp` are needed, since `EXEC` already dispatches generically through the table.

On embedded targets (Pico), indices `0xD0`–`0xFF` are reserved for custom, application-specific native functions — GPIO calls and similar host-specific behavior mapped onto the tail end of a separate `funcTable` array. That reserved range and offset mapping is specific to the Pico build; the desktop VM's `funcMap` in `src/vmfuncmap.cpp` dispatches directly on whatever index `EXEC` carries, with no offset math, so new desktop built-ins can use any unused index without needing to stay clear of `0xD0`–`0xFF`.