#include "vm.h"
#include <stdexcept>
#include <cstring>

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

size_t AddrTranslator::count() {
    return slots.size();
}

void AddrTranslator::freeAll() {
    for(const auto& [idx, slot] : slots) {
        memory->free(slot.h);
    }

    slots.clear();

    for(const auto& [idx, h] : arrays) {
        auto* hdr = static_cast<ArrayHeader*>(memory->deref(h));
        if(hdr) {
            auto* cells = reinterpret_cast<ArrayCell*>(hdr + 1);
            for(uint32_t i = 0; i < hdr->length; i++) freeCell(cells[i]);
        }
        memory->free(h);
    }
    arrays.clear();
}

// ---------------------------------------------------------------------
// Arrays (stored in MemAllocator)
// ---------------------------------------------------------------------
// Block layout: [ArrayHeader][ArrayCell x capacity]
// Cell payload: TAG_INT -> int64 bits, TAG_FLOAT -> double bits,
//               TAG_STRING -> Handle of a NUL-terminated byte block.

static inline ArrayCell* cellsOf(void* block) {
    return reinterpret_cast<ArrayCell*>(
        static_cast<ArrayHeader*>(block) + 1);
}

void AddrTranslator::freeCell(const ArrayCell& c) {
    if(c.tag == TAG_STRING && c.payload != INVALID_HANDLE)
        memory->free(static_cast<Handle>(c.payload));
}

static ArrayCell emptyCell() {
    ArrayCell c;
    c.tag = TAG_INT;
    c.payload = 0;
    return c;
}

void AddrTranslator::arrayWrite(int arrayIndex, int64_t index, const Variant& v) {
    if(index < 0)
        throw std::runtime_error("ARRWRITE: negative index");
    const uint32_t needed = static_cast<uint32_t>(index) + 1;

    // create the array on first write
    auto it = arrays.find(arrayIndex);
    if(it == arrays.end()) {
        uint32_t cap = needed < 4 ? 4 : needed;
        Handle h = memory->alloc(sizeof(ArrayHeader) + cap * sizeof(ArrayCell));
        auto* hdr = static_cast<ArrayHeader*>(memory->deref(h));
        hdr->length = 0;
        hdr->capacity = cap;
        it = arrays.emplace(arrayIndex, h).first;
    }

    auto* hdr = static_cast<ArrayHeader*>(memory->deref(it->second));

    // grow the block if needed (new block, copy cells, free the old one)
    if(needed > hdr->capacity) {
        uint32_t newCap = hdr->capacity;
        while(newCap < needed) newCap *= 2;

        Handle nh = memory->alloc(sizeof(ArrayHeader) + newCap * sizeof(ArrayCell));
        auto* nhdr = static_cast<ArrayHeader*>(memory->deref(nh));
        nhdr->length = hdr->length;
        nhdr->capacity = newCap;
        std::memcpy(cellsOf(nhdr), cellsOf(hdr), hdr->length * sizeof(ArrayCell));

        memory->free(it->second);
        it->second = nh;
        hdr = nhdr;
    }

    ArrayCell* cells = cellsOf(hdr);

    // fill the gap between old length and the target index with empty cells
    for(uint32_t i = hdr->length; i < needed; i++) cells[i] = emptyCell();
    if(needed > hdr->length) hdr->length = needed;

    // build the new cell BEFORE freeing the old one, so a self-overwrite is safe
    ArrayCell nc;
    nc.tag = v.type;
    nc.payload = 0;
    switch(v.type) {
    case TAG_INT: {
        int64_t val = std::get<int64_t>(v.data);
        std::memcpy(&nc.payload, &val, sizeof(val));
        break;
    }
    case TAG_FLOAT: {
        double val = std::get<double>(v.data);
        std::memcpy(&nc.payload, &val, sizeof(val));
        break;
    }
    case TAG_STRING: {
        const std::string& s = std::get<std::string>(v.data);
        Handle sh = memory->alloc(s.size() + 1);
        void* dst = memory->deref(sh);
        std::memcpy(dst, s.data(), s.size());
        static_cast<char*>(dst)[s.size()] = '\0';
        nc.payload = sh;
        // alloc can't move existing blocks (handles are stable), but re-fetch anyway
        hdr = static_cast<ArrayHeader*>(memory->deref(it->second));
        cells = cellsOf(hdr);
        break;
    }
    }

    freeCell(cells[index]);
    cells[index] = nc;
}

Variant AddrTranslator::arrayRead(int arrayIndex, int64_t index) {
    auto it = arrays.find(arrayIndex);
    if(it == arrays.end())
        throw std::runtime_error("ARRREAD: array does not exist");

    auto* hdr = static_cast<ArrayHeader*>(memory->deref(it->second));
    if(index < 0 || static_cast<uint64_t>(index) >= hdr->length)
        throw std::runtime_error("ARRREAD: index out of range");

    const ArrayCell& c = cellsOf(hdr)[index];
    Variant v;
    v.type = c.tag;
    switch(c.tag) {
    case TAG_INT: {
        int64_t val;
        std::memcpy(&val, &c.payload, sizeof(val));
        v.data = val;
        break;
    }
    case TAG_FLOAT: {
        double val;
        std::memcpy(&val, &c.payload, sizeof(val));
        v.data = val;
        break;
    }
    case TAG_STRING:
        v.data = std::string(static_cast<const char*>(memory->deref(static_cast<Handle>(c.payload))));
        break;
    }
    return v;
}