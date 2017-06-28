// vm_alloc_small.h
//
#pragma once
#ifndef __SDL_SYSTEM_MEMORY_VM_ALLOC_SMALL_H__
#define __SDL_SYSTEM_MEMORY_VM_ALLOC_SMALL_H__

#include "dataserver/system/page_head.h"

#if defined(SDL_OS_WIN32) || SDL_DEBUG_SMALL_MEMORY
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

} // mmu
} // sdl
} // db

#endif // #if defined(SDL_OS_WIN32) || SDL_DEBUG_SMALL_MEMORY
#endif // __SDL_SYSTEM_MEMORY_VM_ALLOC_SMALL_H__