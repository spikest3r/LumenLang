# Interactive Debugger

```bash
lumen program.lmn --debugger
```

The program is compiled and then loaded under a command prompt. Execution does not start until you type `run`. Program input (`inputInt()`, `inputStr()`) is read from the same terminal as debugger commands. Compile without `--no-debug` to see variable and array names.

## Commands

| Command | Meaning |
|---|---|
| `run` | Start executing until a breakpoint or the end. |
| `stop` | Stop execution. |
| `breakpoint set <addr>` | Add a breakpoint at a hexadecimal address (as shown by the [disassembler](./disassembler.md)). |
| `breakpoint remove <addr>` | Remove one breakpoint. |
| `breakpoint list` | List breakpoints. |
| `breakpoint clear` | Remove all breakpoints (asks to confirm). |
| `pc` / `pc <addr>` | Show or set the program counter. |
| `step` | Execute one instruction. |
| `continue` | Resume after a breakpoint. |
| `stack` | Show the operand stack, top first. |
| `variables` | Show all variables, then all arrays with their length and contents. |
| `disassemble` | Show the full disassembly. |
| `profiling` | Toggle instruction-level profiling. |
| `memory` | Show memory-allocator statistics. |
| `help` | List the commands. |
| `quit` / `exit` | Leave the debugger (asks to confirm if the program is still running). |

`stack`, `variables` and `step` need a running program: they answer `Execution has not started` before `run` or after the program finished. Stop at a breakpoint first.

## Example

For this program:

```lumen
scores[0] = 10
scores[1] = 20
names[0] = 'ann'
i = 1
x = scores[i] + 1
println(x)
```

```text
>> breakpoint set 28
Breakpoint set at 28
>> run
Executing...
Breakpoint hit!
>> variables
Variables:
  [i] 1
  [x] 21
Arrays:
  [scores] length 2: [10, 20]
  [names] length 1: ['ann']
>> continue
21
Execution finished
```

Arrays are listed by name with every element (strings are quoted). Without debug symbols the names are replaced by numbers.

## Notes

- Breakpoint addresses are instruction addresses; a breakpoint on an address in the middle of an instruction is never hit.
- The debugger executes instructions one at a time and does not run the garbage collector.
