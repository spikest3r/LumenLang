# Interpreter

Work in progress

## TODO: Syntax guideline

## Instruction set (byte code)


| Opcode (Hex) | Instruction | Description |
| :--- | :--- | :--- |
| `0x01` | `RESERVED` | Reserved for future use |
| `0x02` | `POP_VAR` | Pop top of stack and store into variable |
| `0x03` | `PUSH_VAR` | Push variable value onto stack |
| `0x04` | `EXEC` | Call internal function |
| `0x05` | `JUMP` | Unconditional jump to address |
| `0xA0` | `ADD` | Pop two values, add them, push result |
| `0xA1` | `SUB` | Pop two values, subtract them, push result |
| `0xA2` | `MUL` | Pop two values, multiply them, push result |
| `0xA3` | `DIV` | Pop two values, divide them, push result |
| `0xA4` | `POW` | Pop two values, raise to power, push result |
| `0xA5` | `MOD` | Pop two values, modulo (remainder), push result |