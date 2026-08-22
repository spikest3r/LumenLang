# Standard Library

Beyond the core built-ins ([Input & Output](./input-output.md), string conversions), Lumen ships a small standard library covering strings, files, randomness, and HTTP.

## Capabilities

File I/O, random, and HTTP are optional features that a given VM build may or may not implement. See [Capabilities](./capabilities.md) for `assertCapability` and how to check for them.

## File I/O

Requires the `FS` capability.

| Function | Description |
|---|---|
| `openFile path, &handle` | Open a file, storing a handle in `handle` |
| `writeFile data, handle` | Write a string to an open file |
| `readFile &out, handle` | Read the full contents of an open file into `out` |
| `closeFile handle` | Close an open file handle |

## Random

Requires the `random` capability.

| Function | Description |
|---|---|
| `randomSeed seed` | Seed the random number generator |
| `random &out` | Generate a random float in `[0.0, 1.0)` into `out` |
| `randomRange min, max, &out` | Generate a random integer in `[min, max]` (inclusive) into `out` |

## HTTP

Requires the `HTTP` capability.

| Function | Description |
|---|---|
| `httpRequest method, url, headers, body, &status, &response` | Perform an HTTP request |

- `method` — `'GET'`, `'POST'`, `'PUT'`, or `'DELETE'` (case-insensitive)
- `url` — full URL including scheme, e.g. `'http://example.com/path'`
- `headers` — newline-separated `Key: Value` pairs, or `''` for none
- `body` — request body string, ignored for `GET`
- `status` — receives the HTTP status code, or `-1` on a connection-level failure
- `response` — receives the response body, or an error message on failure

HTTPS URLs are not currently supported — only plain `http://` requests.

## String Manipulation

See [Strings](./strings.md#string-manipulation) for `strlen`, `substr`, `strfind`, `strcase`, and `trim`.