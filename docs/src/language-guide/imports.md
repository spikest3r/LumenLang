# Imports

```lumen
import 'lib.lmn'
```

`import` compiles another script into the program at that point. The imported file's top-level statements run when execution reaches the import; its routines and functions become available to the whole program (before or after the import line); its variables, arrays and labels live in the same global scope.

```lumen
# lib.lmn
function twice(x)
  return x * 2
endfunction
println('lib loaded')
```

```lumen
import 'lib.lmn'
println(twice(4))
```

```text
lib loaded
8
```

## Rules

- The path is resolved relative to the directory you run `lumen` from, not relative to the importing file.
- The path is a string literal without parentheses.
- Imports can nest up to five levels deep (`Reached maximum import depth of 5`).
- A file cannot import itself, a missing file is a compile error (`Cannot open imported file '...'`), and defining the same routine or function name in two files is a compile error.
- Everything is compiled into one `.bin`; nothing is looked up at run time.
- Error messages name the file the error is in.
