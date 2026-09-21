# Built-in Functions

Built-ins are compiled to `EXEC <index>`: the arguments are pushed left to right, the VM pops them, runs the native function and pushes the result if there is one. The compiler checks the argument count (`argument count mismatch for 'name': expected N, got M`).

| Index | Function | Args | Returns | Description |
|---|---|---|---|---|
| `0x01` | `println(value)` | 1 | – | Print a value and a newline. |
| `0x02` | `print(value)` | 1 | – | Print a value. |
| `0x03` | `inputInt()` | 0 | integer | Read a whitespace-delimited integer; `0` and a message if invalid. |
| `0x04` | `inputStr()` | 0 | string | Read one whitespace-delimited word. |
| `0x05` | `str2int(text)` | 1 | integer | Parse an integer; `Function error` if invalid. |
| `0x06` | `int2str(n)` | 1 | string | Integer to text; `Function error` for a float. |
| `0x07` | `str2float(text)` | 1 | float | Parse a float; `0` if invalid. |
| `0x08` | `float2str(x)` | 1 | string | Number to text with six decimals. |
| `0xA0` | `assertCapability(name)` | 1 | – | Stop with an error if the host lacks the capability. |
| `0xA1` | `openFile(path)` | 1 | integer | Open (create if missing, keep contents) for read/write; returns a handle. |
| `0xA2` | `writeFile(text, handle)` | 2 | – | Append text to the file and flush. |
| `0xA3` | `readFile(handle)` | 1 | string | Whole file contents from the start. |
| `0xA4` | `closeFile(handle)` | 1 | – | Close and release the handle. |
| `0xA5` | `randomSeed(seed)` | 1 | – | Seed the generator. |
| `0xA6` | `random()` | 0 | float | Float in [0, 1). |
| `0xA7` | `randomRange(min, max)` | 2 | integer | Integer in [min, max]. |
| `0xA8` | `httpRequest(method, url, headers, body, &response)` | 5 | integer | HTTP request; returns status or `-1`. |
| `0xA9` | `strlen(text)` | 1 | integer | Length. |
| `0xAA` | `substr(text, start, length)` | 3 | string | Substring. |
| `0xAB` | `strfind(text, needle)` | 2 | integer | Index of first match or `-1`. |
| `0xAC` | `strcase(text, upper)` | 2 | string | Upper case if `upper` is non-zero, else lower. |
| `0xAD` | `trim(text)` | 1 | string | Strip leading and trailing whitespace. |

Failures inside a native function are caught by the VM: it prints `Function error`, then the message, and halts.

## Adding a built-in

1. Implement it in `src/vm/vmfuncmap.cpp` under a new index. Pop the arguments in reverse order (last argument on top) and push the result if there is one.
2. Register it in `funcList` in `src/compiler/compiler_tables.cpp` with `{index, argCount, returnable}`.

The disassembler and debug symbols pick the name up from `funcList` automatically.
