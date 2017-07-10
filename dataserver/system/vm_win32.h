// vm_win32.h
//
#pragma once
#ifndef __SDL_SYSTEM_VM_WIN32_H__
#define __SDL_SYSTEM_VM_WIN32_H__

#include "dataserver/system/vm_base.h"

#if defined(SDL_OS_WIN32)

namespace sdl { namespace db {

class vm_win32 final : public vm_base {
public:
    size_t const byte_reserved;
    size_t const page_reserved;
    size_t const slot_reserved;
    size_t const block_reserved;
public:
    vm_win32(size_t, bool commited);
    ~vm_win32();
    char * base_address() const {
        return m_base_address;
    }
    char * end_address() const {
        return m_base_address + byte_reserved;
    }
    bool is_open() const {
        return m_base_address != nullptr;
    }
    bool is_alloc(char * start, size_t) const;
    char * alloc(char * start, size_t);
    char * alloc_all() {
        return alloc(base_address(), byte_reserved);
    }
    bool release(char * start, size_t);
    bool release_all() {
        return release(base_address(), byte_reserved);
    }
private:
    bool assert_address(char const * const start, size_t const size) const {
        SDL_ASSERT(start);
        SDL_ASSERT(size && !(size % page_size));
        SDL_ASSERT(m_base_address <= start);
        SDL_ASSERT(start + size <= end_address());
        return true;
    }
    static char * init_vm_alloc(size_t, bool);
    size_t last_block() const {
        return block_reserved - 1;
    }
    size_t last_block_page_count() const {
        const size_t n = page_reserved % block_page_num;
        return n ? n : block_page_num;
    }
    size_t last_block_size() const {
        return page_size * last_block_page_count();
    }
    size_t alloc_block_size(const size_t b) const {
        SDL_ASSERT(b < block_reserved);
        if (b == last_block())
            return last_block_size();
        return block_size;
    }
private:
    char * const m_base_address = nullptr;
    std::vector<bool> m_block_commit;
};

} // sdl
} // db

#endif // SDL_OS_WIN32
#endif // __SDL_SYSTEM_VM_WIN32_H__
