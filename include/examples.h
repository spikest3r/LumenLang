#pragma once
#include "includes.h"

struct Example {
    std::string* codePtr;
    std::string fileName;
};

std::string fizzbuzz = R"(N = 0

print 'N='
inputInt &N
println ''

i = 1

label loop

i15 = i % 15

if i15 == 0
    println 'FizzBuzz'
else
    i3 = i % 3

    if i3 == 0
        println 'Fizz'
    else
        i5 = i % 5

        if i5 == 0
            println 'Buzz'
        else
            println i
        endif
    endif
endif

i = i + 1

if i <= N
    jump loop
endif
)";

std::string temperature = R"(temp = 0
result = 0

routine c2f
result = temp * 9 / 5 + 32
endroutine

routine f2c
result = temp - 32
result = result * 5 / 9
endroutine

routine ask
print 'Temparature: '
endroutine

routine show
print 'Result: '
println result
endroutine

println '1. C to F'
println '2. F to C'
print 'Select mode '
mode = 0
inputInt &mode
if mode == 1
call ask
inputInt &temp
call c2f
call show
else
if mode == 2
call ask
inputInt &temp
call f2c
call show
else
println 'Incorrect mode'
endif
endif
)";

std::string age = R"(yearNow = 2026
userYear = 0
println 'Hello, world!'
print 'Enter your birth year: '
inputInt &userYear
age = yearNow - userYear
print 'Your age: '
println age
)";

std::string infinite_loop = R"(label repeat
println 'Hello, world!'
jump repeat
)";

std::string blinky = R"(gpioInit 25
gpioSetDir 25 1

wait = 500

direction = 1
increment = 100

label loop

gpioPut 25 1
sleepMs wait
gpioPut 25 0
sleepMs wait

wait = wait + increment * direction
if wait >= 1000
direction = -1
endif
if wait <= 100
direction = 1
endif

println wait

jump loop
)";

std::string buttons = R"(gpioInit 25
gpioSetDir 25 1

gpioInit 2
gpioSetDir 2 0
gpioPullUp 2

println 'GPIO Init'

btnVal = 0

led = 0

label loop

gpioGet 2 &btnVal
btnVal = 1 - btnVal

if btnVal == 1

println 'Pressed'

led = 1 - led
gpioPut 25 led
println led

label await

gpioGet 2 &btnVal
btnVal = 1 - btnVal

if btnVal == 1
jump await
endif

println 'Released'

endif

sleepMs 50

jump loop
)";

std::unordered_map<std::string, Example> exampleMap = {
    {"age", {&age, "age.lmn"}},
    {"infinite-loop", {&infinite_loop, "infinite.lmn"}},
    {"temperature", {&temperature, "temperature.lmn"}},
    {"fizzbuzz", {&fizzbuzz, "fizzbuzz.lmn"}},
    {"blinky", {&blinky, "blinky.lmn"}},
    {"buttons", {&buttons, "buttons.lmn"}}
};