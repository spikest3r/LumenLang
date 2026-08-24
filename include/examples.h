#pragma once
#include "includes.h"

struct Example {
    std::string* codePtr;
    std::string fileName;
};

std::string fizzbuzz = R"(print 'N='
inputInt &x
repeat x, n
n = n + 1
if n % 15 == 0
println 'FizzBuzz'
elif n % 5 == 0
println 'Buzz'
elif n % 3 == 0
println 'Fizz'
else
println n
endif
endrepeat
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
elif mode == 2
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

std::unordered_map<std::string, Example> exampleMap = {
    {"age", {&age, "age.lmn"}},
    {"infinite-loop", {&infinite_loop, "infinite.lmn"}},
    {"temperature", {&temperature, "temperature.lmn"}},
    {"fizzbuzz", {&fizzbuzz, "fizzbuzz.lmn"}}
};