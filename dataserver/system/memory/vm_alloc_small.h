// vm_alloc_small.h
//
#pragma once
#ifndef __SDL_SYSTEM_MEMORY_VM_ALLOC_SMALL_H__
#define __SDL_SYSTEM_MEMORY_VM_ALLOC_SMALL_H__

#include "dataserver/system/page_head.h"

#if defined(SDL_OS_WIN32)
namespace sdl { namespace db { namespace mmu {

class vm_alloc_small: noncopyable {
    using this_error = sdl_exception_t<vm_alloc_small>;
public:
    enum { page_size = page_head::page_size }; // 8192 byte
    uint64 const byte_reserved;
    uint64 const page_reserved;
    explicit vm_alloc_small(uint64);
    void * alloc(uint64 start, uint64 size);
    bool clear(uint64 start, uint64 size);
private:
    bool check_address(uint64 start, uint64 size) const;
private:
    using slot_t = std::unique_ptr<char[]>;
    std::vector<slot_t> slots;  // prototype for small memory only
};

inline vm_alloc_small::vm_alloc_small(uint64 const size)
    : byte_reserved(size)
    , page_reserved(size / page_size)
{
    SDL_ASSERT(size && !(size % page_size));
    SDL_ASSERT(page_reserved);
    slots.resize(page_reserved);
}

inline bool vm_alloc_small::check_address(uint64 const start, uint64 const size) const {
    const size_t index = start / page_size;
    if ((index < page_reserved) && (start + size <= byte_reserved)) {
        return true;
    }
    throw_error<this_error>("bad free");
    return false;
}

inline void * vm_alloc_small::alloc(uint64 const start, uint64 const size)
{
    SDL_ASSERT(start + size <= byte_reserved);
    SDL_ASSERT(size && !(size % page_size));
    SDL_ASSERT(!(start % page_size));
    if (check_address(start, size)) {
        auto & p = slots[start / page_size];
        if (!p){
            p.reset(new char[page_size]);
        }
        return p.get();
    }
    return nullptr;
}

inline bool vm_alloc_small::clear(uint64 const start, uint64 const size)
{
    SDL_ASSERT(start + size <= byte_reserved);
    SDL_ASSERT(size && !(size % page_size));
    SDL_ASSERT(!(start % page_size));
    if (check_address(start, size)) {
        auto & p = slots[start / page_size];
        if (p) {
            p.reset();
            return true;
        }
    }
    SDL_ASSERT(0);
    return false;
}

} // mmu
} // sdl
} // db

#endif // #if defined(SDL_OS_WIN32)
#endif // __SDL_SYSTEM_MEMORY_VM_ALLOC_SMALL_H__