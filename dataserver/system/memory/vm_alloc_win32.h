// vm_alloc_win32.h
//
#pragma once
#ifndef __SDL_SYSTEM_MEMORY_VM_ALLOC_WIN32_H__
#define __SDL_SYSTEM_MEMORY_VM_ALLOC_WIN32_H__

#include "dataserver/system/page_head.h"

#if defined(SDL_OS_WIN32)

/*#if !defined(NOMINMAX)
//warning: <windows.h> defines macros min and max which conflict with std::numeric_limits !
#define NOMINMAX
#endif*/

#include <windows.h>

namespace sdl { namespace db { namespace mmu {

class vm_alloc_win32: noncopyable {
    using this_error = sdl_exception_t<vm_alloc_win32>;
public:
    enum { page_size = page_head::page_size }; // 8192 byte
    uint64 const byte_reserved;
    uint64 const max_page;
    explicit vm_alloc_win32(uint64);
    ~vm_alloc_win32();
    void * alloc(uint64 start, uint64 size);
    void clear(uint64 start, uint64 size);
private:
    void * base_address() const {
        SDL_ASSERT(m_base_address);
        return m_base_address;
    }
    void * m_base_address = nullptr;
};

} // mmu
} // sdl
} // db

#endif // #if defined(SDL_OS_WIN32)
#endif // __SDL_SYSTEM_MEMORY_VM_ALLOC_WIN32_H__
