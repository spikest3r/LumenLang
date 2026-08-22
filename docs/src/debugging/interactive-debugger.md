# Interactive Debugger

Lumen has a built-in step debugger (`src/debugvm.cpp`), reached via `--debugger` (which requires `--run`):

```bash
lumen program.lmn --compile --dbgsym --run --debugger
```

Compiling with `--dbgsym` first is optional but strongly recommended — without it, the debugger shows raw slot indices instead of your variable and routine names.

## Starting up

```
Debugger active. 'help' for list of commands
>>
```

## Commands

| Command | Description |
|---|---|
| `run` | Begin bytecode execution |
| `stop` | Stop execution |
| `breakpoint set <address>` | Set a breakpoint at a bytecode offset |
| `breakpoint remove <address>` | Remove a breakpoint |
| `breakpoint list` | List all breakpoints |
| `breakpoint clear` | Remove all breakpoints |
| `pc` | Print the current program counter |
| `pc <address>` | Set the program counter |
| `step` / `s` | Execute a single instruction |
| `continue` | Resume execution until the next breakpoint or halt |
| `stack` | Print the operand stack, top to bottom |
| `variables` | Print all variable slots and their current values |
| `disassemble` | Show the disassembled bytecode, with the current PC marked |
| `help` | Show the command list |

## A typical session

```
>> run
Executing...
Breakpoint hit!
>> stack
Stack (top to bottom):
  [2] {int64: 5}  <- top
  [1] {int64: 3}
  [0] {string: "hello"}
>> variables
Variables:
  [i] {int64: 5}
  [result] {int64: 8}
>> step
>> continue
Execution finished
```

Addresses for `breakpoint` and `pc` are bytecode offsets, not source line numbers — pair `--debugger` with `--disassemble` (or the debugger's own `disassemble` command) to find the offset you want to break at. If you compiled with `--dbgsym`, routine names shown by `disassemble` will help you find the offset where a particular routine begins.
