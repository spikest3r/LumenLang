#include "vm.h"

AddrTranslator::AddrTranslator(MemAllocator* allocator) {
    memory = allocator;
}

// AddrTranslator::~AddrTranslator() {}

const Slot* AddrTranslator::readSlot(int index) {
    auto it = slots.find(index);
    if(it != slots.end()) {
        return &it->second;
    }
    return nullptr;
}

void AddrTranslator::allocateSlotSize(int index, size_t size, TypeTag tag) {
    bool allocate = true;

    // check 2 requirements
    // firstly, does the slot exist
    auto it = slots.find(index);
    if(it != slots.end()) {
        // exists, check size
        Slot& s = it->second;
        if(static_cast<size_t>(s.size) < size) {
            // doesnt meet the requirement, free
            memory->free(s.h);
        } else {
            // everything alright, dont allocate
            allocate = false;
        }
    }

    if(allocate) {
        Handle h = memory->alloc(size);
        void* p = memory->deref(h);
        memset(p, 0xAD, size); // pattern for freshly allocated memory
        slots[index] = Slot {h, static_cast<int>(size), tag};
    } else {
        // just update the tag
        slots[index].tag = tag;
    }
}