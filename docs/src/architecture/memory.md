# Memory & Garbage Collection

Variables, array elements and string values live in a managed heap instead of a fixed array of values.

## Layers

| Layer | File | Role |
|---|---|---|
| `MemAllocator` | `src/vm/memoryallocator.cpp` | Paged allocator (4 KiB pages; bigger blocks get their own page). Hands out **handles**, not raw pointers, so blocks can move. |
| `AddrTranslator` | `src/vm/translator.cpp` | Maps a variable slot number to a slot (handle, size, type tag) and an array number to an array block. Reads and writes go through it. |
| VM | `src/vm/vm.cpp` | Uses the translator for `PUSH`/`POP`/`CPY`/`INCV`/`ARRREAD`/`ARRWRITE`. |

Slots are created on the first write to a variable. Reading a variable that was never written yields integer `0`. Arrays are created by the first `ARRWRITE` and grow on demand; an array block starts with a header holding its length followed by its elements, and string elements own their own heap blocks.

## Collector

A mark-and-sweep collector:

1. **Mark** every handle reachable from variable slots and from array blocks (including string elements).
2. **Sweep** (`collect`) frees unmarked blocks and returns empty pages.
3. **Defragment** compacts live objects and rewrites the handle table.

Values on the operand stack are copied `Variant`s and are not roots.

**Trigger.** Every 1000 instructions `run()` compares allocated bytes with a threshold. It starts at 1 MiB; after each collection it becomes twice the live size, never below 1 MiB.

## Observing memory

The [debugger](../debugging/interactive-debugger.md) has `memory` for allocator statistics and `variables` for variables and arrays.
