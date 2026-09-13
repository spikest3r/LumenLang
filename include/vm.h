#pragma once
#include "includes.h"
#include "types.h"

// MemAllocator (memoryallocator.cpp)

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

using Handle = uint32_t;
constexpr Handle INVALID_HANDLE = 0;

class MemAllocator {
public:
    static constexpr size_t PAGE_SIZE = 4096;

    MemAllocator();
    ~MemAllocator();

    // --- allocation API ---
    Handle alloc(size_t size);       // returns a stable handle, not a raw pointer
    void   free(Handle h);

    // --- handle <-> raw pointer ---
    void*  deref(Handle h);          // current live address for this handle (nullptr if invalid)
    bool   valid(Handle h) const;

    // --- GC hooks ---
    void mark(Handle h);             // mark reachable during trace
    void collect();                  // sweep unmarked, return dead slots/pages
    void defragment();               // compact live objects, rewrite handle table, coalesce free pages

    // --- introspection ---
    size_t allocated_bytes() const;
    size_t free_page_count() const;

private:
    struct Page {
        void*  base;        // raw memory block
        size_t capacity;    // PAGE_SIZE or multiple thereof
        size_t bump_offset; // next free byte within page (bump allocation)
        size_t live_count;  // number of live objects in this page
        bool   is_large;    // true if this page/run backs a single large object
    };

    struct AllocHeader {
        void*  ptr;        // current raw address (rewritten by defragment())
        size_t size;       // requested size
        Page*  page;       // owning page
        bool   marked;     // GC mark bit
    };

    // handle -> metadata; this IS the translation layer
    std::unordered_map<Handle, AllocHeader> owned_;

    // pages currently handing out memory
    std::vector<Page*> active_pages_;

    // fully-empty pages available for reuse
    std::vector<Page*> free_pages_;

    Handle next_handle_ = 1; // 0 reserved as INVALID_HANDLE

    Page* allocate_new_page(size_t min_size);
    Page* find_page_with_space(size_t size);
    void  release_page(Page* page);
};

// AddrTranslator (translator.cpp)

struct Slot {
    Handle h;
    int size;
    TypeTag tag; // for internal reference
};

class AddrTranslator {
public:
    AddrTranslator(MemAllocator* allocator);
    // ~AddrTranslator(); // TODO

    const Slot* readSlot(int index); // nullptr on bad slot idx
    void allocateSlotSize(int index, size_t size, TypeTag tag);
    size_t count();
    void freeAll();
private:
    MemAllocator* memory;

    std::unordered_map<int, Slot> slots;
};

// vm

struct CallFrame {
    int returnPC;
    int routineBase;
};

class VMExecutionData {
public:
    VMExecutionData() : translator(&memory) {}

    MemAllocator memory;
    AddrTranslator translator;

    std::vector<Variant> stack;
    std::vector<CallFrame> pcStack;

    int PC = 0;
    int routineBase = 0;
    bool halt = false;
};

using NativeFn = std::function<void(VMExecutionData*)>;

int64_t getInt(const Variant& v);
double getNumeric(const Variant& v); // reads int64_t or double as a double, for mixed int/float arithmetic
bool isFloatVariant(const Variant& a, const Variant& b); // true if either operand is TAG_FLOAT

extern std::unordered_map<int, NativeFn> funcMap;

int run(
    VMProgramData* progData
);

int run_debug(
    VMProgramData* progData
);

int execute(
    VMProgramData* progData,
    VMExecutionData* execData
);