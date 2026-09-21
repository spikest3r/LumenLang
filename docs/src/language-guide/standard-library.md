# Standard Library

Built-in functions are called like any function. Those that produce a value are used inside expressions; the others are statements. The complete table with argument lists is in the [Built-in Functions reference](../reference/builtin-functions.md).

## Console

`print(value)`, `println(value)`, `inputInt()`, `inputStr()`, see [Input & Output](./input-output.md).

## Conversion

```lumen
n = str2int('42')        # 42
f = str2float('3.5')     # 3.5
s = int2str(42)          # '42'
t = float2str(3.5)       # '3.500000'
```

- `str2int` stops the program with `Function error` if the text is not an integer (`str2int('abc')`).
- `str2float` returns `0` for text that is not a number.
- `int2str` needs an integer; a float argument is a `Function error`.
- `float2str` accepts an integer or a float.

## Strings

```lumen
strlen('hello')                  # 5
substr('hello world', 6, 5)      # 'world'   (text, start, length)
strfind('hello world', 'world')  # 6, or -1 when absent
strcase('Hello', 1)              # 'HELLO'   (non-zero: upper, 0: lower)
trim('  hi  ')                   # 'hi'
```

`substr` returns an empty string when `start` is past the end.

## Random numbers

```lumen
randomSeed(42)
r = random()             # float in [0, 1)
dice = randomRange(1, 6) # integer from 1 to 6, inclusive
```

## Files

```lumen
assertCapability('FS')
h = openFile('log.txt')
writeFile('line one\n', h)
writeFile('line two\n', h)
println(readFile(h))
closeFile(h)
```

- `openFile(path)` opens the file for reading and writing, creating it if it does not exist. Existing contents are kept. It returns a handle.
- `writeFile(text, handle)` appends `text` to the end of the file and flushes it.
- `readFile(handle)` returns the whole file, from the start.
- `closeFile(handle)` closes the handle. Always close files you open.

## HTTP

```lumen
assertCapability('HTTP')
body = ''
status = httpRequest('GET', 'http://example.com/', '', '', &body)
println(status)
println(body)
```

`httpRequest(method, url, headers, body, &response)`:

- `method` is `GET`, `POST`, `PUT` or `DELETE`.
- `headers` is a string of `Name: value` lines separated by `\n` (empty for none). A `Content-Type` header sets the type of the request body.
- `body` is the request body (empty for GET).
- `&response` is a [reference](./references.md) to the variable that receives the response body.
- It returns the HTTP status code, or `-1` if the request could not be made, in which case the response variable holds an error message.

## Capabilities

`assertCapability(name)`, see [Capabilities](./capabilities.md).

## Errors

If a built-in cannot do its job (wrong argument type, unknown handle...), the program prints `Function error` followed by a message and stops.
