# Temperature Converter

```bash
lumen --examples temperature
```

```lumen
function c2f(temp)
return temp * 9 / 5 + 32
endfunction

function f2c(temp)
result = temp - 32
return result * 5 / 9
endfunction

routine ask()
print('Temparature: ')
endroutine

routine show(result)
print('Result: ')
println(result)
endroutine

println('1. C to F')
println('2. F to C')
print('Select mode ')
mode = inputInt()
if mode == 1
ask()
temp = inputInt()
result = c2f(temp)
show(result)
elif mode == 2
ask()
temp = inputInt()
result = f2c(temp)
show(result)
else
println('Incorrect mode')
endif
```

Two **functions** compute a value and `return` it; two **routines** print. Definitions come first, but they could equally follow the main code. Parameters are global variables, so `temp` in the main program and the `temp` parameter are the same variable.

Touches: [routines & functions](../language-guide/routines.md), [conditionals](../language-guide/conditionals.md).

## Scan into Android

![QR for the temperature example](../images/qrcodes/temperature.png)
