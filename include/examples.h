#pragma once
#include "includes.h"

struct Example {
std::string* codePtr;
std::string fileName;
};

std::string fizzbuzz = R"(print('N=')
x = inputInt()
repeat x, n
n = n + 1
if n % 15 == 0
println('FizzBuzz')
elif n % 5 == 0
println('Buzz')
elif n % 3 == 0
println('Fizz')
else
println(n)
endif
endrepeat
)";

std::string temperature = R"(function c2f(temp)
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
)";

std::string age = R"(yearNow = 2026
userYear = 0
println('Hello, world!')
print('Enter your birth year: ')
userYear = inputInt()
age = yearNow - userYear
print('Your age: ')
println(age)
)";

std::string infinite_loop = R"(label loop
println('Hello, world!')
jump loop
)";

std::unordered_map<std::string, Example> exampleMap = {
    {"age", {&age, "age.lmn"}},
    {"infinite-loop", {&infinite_loop, "infinite.lmn"}},
    {"temperature", {&temperature, "temperature.lmn"}},
    {"fizzbuzz", {&fizzbuzz, "fizzbuzz.lmn"}}
};