// vm_alloc_win32.h
//
#pragma once
#ifndef __SDL_SYSTEM_MEMORY_VM_ALLOC_WIN32_H__
#define __SDL_SYSTEM_MEMORY_VM_ALLOC_WIN32_H__

#include "dataserver/system/page_head.h"

#if defined(SDL_OS_WIN32)

namespace sdl { namespace db { namespace mmu {

class vm_alloc_win32: noncopyable {
    using this_error = sdl_exception_t<vm_alloc_win32>;
public:
    enum { page_size = page_head::page_size }; // 8192 byte
    uint64 const byte_reserved;
    uint64 const max_page;
    explicit vm_alloc_win32(uint64);
    ~vm_alloc_win32(){}
    void * alloc(uint64 start, uint64 size);
    void clear(uint64 start, uint64 size);
private:
};

inline vm_alloc_win32::vm_alloc_win32(uint64 const size)
    : byte_reserved(size)
    , max_page(size / page_size)
{
    SDL_ASSERT(size && !(size % page_size));
    SDL_ASSERT(max_page);
}

inline void * vm_alloc_win32::alloc(uint64 const start, uint64 const size)
{
    SDL_ASSERT(start + size <= byte_reserved);
    SDL_ASSERT(size && !(size % page_size));
    SDL_ASSERT(!(start % page_size));
    const size_t index = start / page_size;
    if (max_page <= index) {
        throw_error<this_error>("bad alloc");
    }
    SDL_ASSERT(0);
    return nullptr;
}

inline void vm_alloc_win32::clear(uint64 const start, uint64 const size)
{
    SDL_ASSERT(start + size <= byte_reserved);
    SDL_ASSERT(size && !(size % page_size));
    SDL_ASSERT(!(start % page_size));
    const size_t index = start / page_size;
    if (max_page <= index) {
        throw_error<this_error>("bad page");
    }
    SDL_ASSERT(0);
}

} // mmu
} // sdl
} // db

#endif // #if defined(SDL_OS_WIN32)
#endif // __SDL_SYSTEM_MEMORY_VM_ALLOC_WIN32_H__
