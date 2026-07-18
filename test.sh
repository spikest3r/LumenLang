#!/usr/bin/env bash

set -e

echo "FizzBuzz test"

SCRIPT="./build/lumen"
FILE="examples/fizzbuzz.lmn"

EXPECTED=$(cat << 'EOF'
N=
1
2
Fizz
4
Buzz
Fizz
7
8
Fizz
Buzz
11
Fizz
13
14
FizzBuzz
16
17
Fizz
19
Buzz
EOF
)

# run program with input 20
OUTPUT=$(printf "20\n" | "$SCRIPT" "$FILE")

# normalize (remove weird CRLF issues just in case)
OUTPUT=$(echo "$OUTPUT" | sed 's/\r//g')
EXPECTED=$(echo "$EXPECTED" | sed 's/\r//g')

echo "===== OUTPUT ====="
echo "$OUTPUT"
echo "=================="

if [[ "$OUTPUT" == "$EXPECTED" ]]; then
    echo "PASSED"
    exit 0
else
    echo "FAILED"
    exit 1
fi