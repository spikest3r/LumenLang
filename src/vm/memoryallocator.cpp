#include "vm.h"
#include <cstdlib>
#include <cstring>
#include <algorithm>

MemAllocator::MemAllocator() = default;

MemAllocator::~MemAllocator() {
    for (Page* p : active_pages_) {
        std::free(p->base);
        delete p;
    }
    for (Page* p : free_pages_) {
        std::free(p->base);
        delete p;
    }
}

// ---------------------------------------------------------------------
// Page management
// ---------------------------------------------------------------------

MemAllocator::Page* MemAllocator::allocate_new_page(size_t min_size) {
    size_t capacity = std::max(min_size, ALLOC_PAGE_SIZE);

    for (auto it = free_pages_.begin(); it != free_pages_.end(); ++it) {
        if ((*it)->capacity >= capacity) {
            Page* page = *it;
            free_pages_.erase(it);
            page->bump_offset = 0;
            page->live_count = 0;
            page->is_large = (min_size > ALLOC_PAGE_SIZE);
            active_pages_.push_back(page);
            return page;
        }
    }

    Page* page = new Page();
    page->base = std::malloc(capacity);
    page->capacity = capacity;
    page->bump_offset = 0;
    page->live_count = 0;
    page->is_large = (min_size > ALLOC_PAGE_SIZE);

    active_pages_.push_back(page);
    return page;
}

MemAllocator::Page* MemAllocator::find_page_with_space(size_t size) {
    for (Page* page : active_pages_) {
        if (page->is_large) continue;
        if (page->capacity - page->bump_offset >= size) {
            return page;
        }
    }
    return nullptr;
}

void MemAllocator::release_page(Page* page) {
    active_pages_.erase(
        std::remove(active_pages_.begin(), active_pages_.end(), page),
        active_pages_.end()
    );
    page->bump_offset = 0;
    page->live_count = 0;
    free_pages_.push_back(page);
}

// ---------------------------------------------------------------------
// Allocation API
// ---------------------------------------------------------------------

Handle MemAllocator::alloc(size_t size) {
    if (size == 0) return INVALID_HANDLE;

    Page* page = find_page_with_space(size);
    if (!page) {
        page = allocate_new_page(size);
    }

    void* ptr = static_cast<char*>(page->base) + page->bump_offset;
    page->bump_offset += size;
    page->live_count += 1;

    Handle h = next_handle_++;

    AllocHeader hdr;
    hdr.ptr = ptr;
    hdr.size = size;
    hdr.page = page;
    hdr.marked = false;
    owned_[h] = hdr;

    return h;
}

void MemAllocator::free(Handle h) {
    auto it = owned_.find(h);
    if (it == owned_.end()) return;

    Page* page = it->second.page;
    owned_.erase(it);

    page->live_count -= 1;
    if (page->live_count == 0) {
        release_page(page);
    }
    // Bytes inside a still-live page are not physically reclaimed here —
    // bump allocation leaves a hole until collect()/defragment() runs.
}

// ---------------------------------------------------------------------
// Handle <-> raw pointer
// ---------------------------------------------------------------------

void* MemAllocator::deref(Handle h) {
    auto it = owned_.find(h);
    if (it == owned_.end()) return nullptr;
    return it->second.ptr;
}

bool MemAllocator::valid(Handle h) const {
    return owned_.find(h) != owned_.end();
}

// ---------------------------------------------------------------------
// GC hooks
// ---------------------------------------------------------------------

void MemAllocator::mark(Handle h) {
    auto it = owned_.find(h);
    if (it == owned_.end()) return;
    if (it->second.marked) return;
    it->second.marked = true;
    // Children traversal (e.g. array elements holding other Handles)
    // is the VM tracer's job — it should call mark() again per nested handle.
}

void MemAllocator::collect() {
    for (auto it = owned_.begin(); it != owned_.end(); ) {
        AllocHeader& hdr = it->second;
        if (!hdr.marked) {
            Page* page = hdr.page;
            page->live_count -= 1;
            it = owned_.erase(it);
            if (page->live_count == 0) {
                release_page(page);
            }
        } else {
            hdr.marked = false;
            ++it;
        }
    }
}

void MemAllocator::defragment() {
    std::vector<Page*> pages_to_compact = active_pages_;

    for (Page* page : pages_to_compact) {
        if (page->is_large) continue;
        if (page->live_count == 0) continue;
        if (page->bump_offset == 0) continue;

        Page* target = allocate_new_page(page->capacity);
        size_t write_offset = 0;

        for (auto& [h, hdr] : owned_) {
            if (hdr.page != page) continue;

            void* new_ptr = static_cast<char*>(target->base) + write_offset;
            std::memcpy(new_ptr, hdr.ptr, hdr.size);
            write_offset += hdr.size;

            // Rewrite in place — no re-keying needed, since the map key
            // is the Handle, not the address. This is the whole payoff
            // of the handle indirection: no external fixup pass required.
            hdr.ptr = new_ptr;
            hdr.page = target;
        }

        target->bump_offset = write_offset;
        target->live_count = page->live_count;

        release_page(page);
    }
}

// ---------------------------------------------------------------------
// Introspection
// ---------------------------------------------------------------------

size_t MemAllocator::allocated_bytes() const {
    size_t total = 0;
    for (const auto& [h, hdr] : owned_) {
        total += hdr.size;
    }
    return total;
}

size_t MemAllocator::free_page_count() const {
    return free_pages_.size();
}