// vm_alloc_small.h
//
#pragma once
#ifndef __SDL_SYSTEM_MEMORY_VM_ALLOC_SMALL_H__
#define __SDL_SYSTEM_MEMORY_VM_ALLOC_SMALL_H__

#include "dataserver/system/page_head.h"

#if !SDL_DEBUG_SMALL_MEMORY
#error SDL_DEBUG_SMALL_MEMORY
#endif

namespace sdl { namespace db { namespace mmu {

class vm_alloc_small: noncopyable {
    using this_error = sdl_exception_t<vm_alloc_small>;
public:
    enum { page_size = page_head::page_size }; // 8192 byte
    uint64 const byte_reserved;
    uint64 const max_page;
    explicit vm_alloc_small(uint64);
    void * alloc(uint64 start, uint64 size);
    void clear(uint64 start, uint64 size);
private:
    using slot_t = std::unique_ptr<char[]>;
    std::vector<slot_t> slots;  // prototype for small memory only
};

inline vm_alloc_small::vm_alloc_small(uint64 const size)
    : byte_reserved(size)
    , max_page(size / page_size)
{
    SDL_ASSERT(size && !(size % page_size));
    SDL_ASSERT(max_page);
    slots.resize(max_page);
}

inline void * vm_alloc_small::alloc(uint64 const start, uint64 const size)
{
    SDL_ASSERT(start + size <= byte_reserved);
    SDL_ASSERT(size && !(size % page_size));
    SDL_ASSERT(!(start % page_size));
    const size_t index = start / page_size;
    if (slots.size() <= index) {
        throw_error<this_error>("bad alloc");
    }
    auto & p = slots[index];
    if (!p){
        p.reset(new char[page_size]);
    }
    return p.get();
}

inline void vm_alloc_small::clear(uint64 const start, uint64 const size)
{
    SDL_ASSERT(start + size <= byte_reserved);
    SDL_ASSERT(size && !(size % page_size));
    SDL_ASSERT(!(start % page_size));
    const size_t index = start / page_size;
    if (slots.size() <= index) {
        throw_error<this_error>("bad page");
    }
    slots[index].reset();
}

} // mmu
} // sdl
} // db

#endif // __SDL_SYSTEM_MEMORY_VM_ALLOC_SMALL_H__