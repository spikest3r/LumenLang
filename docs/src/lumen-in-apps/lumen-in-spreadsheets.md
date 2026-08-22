# Lumen in [Spreadsheets](https://github.com/spikest3r/Spreadsheets/tree/main)

[Spreadsheets](https://github.com/spikest3r/Spreadsheets/tree/main) embeds the Lumen VM as a scripting backend, similar in spirit to VBA in Excel. Scripts are written and run from the **ScriptingPanel**, and can read and write cells directly through two host functions.

## Host functions

[Spreadsheets](https://github.com/spikest3r/Spreadsheets/tree/main) currently exposes two functions on top of the base language:

| Statement | Description |
|---|---|
| `getCell <row> <col> &<var>` | Reads the value of a cell into a variable |
| `setCell <row> <col> <value>` | Writes a value into a cell |

Both take a 1-based row and column. `getCell` follows the same `&var` convention as [`inputInt`/`inputStr`](./input-output.md#input) — the target variable is passed by reference and written into directly. `setCell` takes the value to write as its third argument, which can be a literal, a variable, or an expression.

```lumen
value = 0
getCell 1 1 &value

setCell 1 2 'done'
```

Cell values read with `getCell` come back as strings — use [`str2int`](./strings.md) to convert before doing arithmetic or numeric comparisons on them.

## Running a script

Scripts live in the ScriptingPanel and are run with the **Run** button. Output from `print`/`println` goes to the console pane below the editor; the script runs to completion (or until an infinite loop is manually stopped) before control returns to the sheet.

## Example: grading a column

This script reads a passing threshold, then walks down column 1 grading each row into column 2:

```lumen
i = 1
x = 0
inputInt &x
println 'Grading column 1'

label repeat
value = 0
getCell i 1 &value
str2int value &value

if value >= 60
    setCell i 2 'PASS'
else
    setCell i 2 'FAIL'
endif

if i < x
    i = i + 1
    jump repeat
endif

println 'Grading completed'
```

This follows the same `label`/`jump` loop pattern described in [Labels & Jumps](./labels-and-jumps.md), with `getCell`/`setCell` standing in for a body that would otherwise just print.

## Example: FizzBuzz into a column

Host functions compose with ordinary Lumen control flow — this script fills column 3 with FizzBuzz output for rows 1 through 15, using the [nested `if`/`else` pattern](./conditionals.md#nesting) since Lumen has no `elseif`:

```lumen
i = 1

label loop

i15 = i % 15

if i15 == 0
    setCell i 3 'FizzBuzz'
else
    i3 = i % 3

    if i3 == 0
        setCell i 3 'Fizz'
    else
        i5 = i % 5

        if i5 == 0
            setCell i 3 'Buzz'
        else
            setCell i 3 i
        endif
    endif
endif

if i < 15
    i = i + 1
    jump loop
endif
```

## Multiple scripts

The ScriptingPanel supports more than one script per file, managed from the **Script** menu: **New**, **Rename**, and **Remove**, alongside a **Scripts** submenu for switching between them. Each script is stored independently and saved with the spreadsheet file.