// vm_malloc.h
//
#pragma once
#ifndef __SDL_MEMORY_VM_MALLOC_H__
#define __SDL_MEMORY_VM_MALLOC_H__

#include "dataserver/system/page_head.h"
#include <bitset>

namespace sdl { namespace db { namespace mmu {

class vm_malloc: noncopyable {
    using this_error = sdl_exception_t<vm_malloc>;
public:
    enum { max_commit_page = 1 + uint16(-1) }; // 65536 pages
    enum { page_size = page_head::page_size }; // 8192 byte
    uint64 const byte_reserved;
    uint64 const page_reserved;
    explicit vm_malloc(uint64);
    bool is_open() const {
        return !!m_base_address;
    }
    void * alloc(uint64 start, uint64 size);
    bool clear(uint64 start, uint64 size);
private:
    bool check_address(uint64 start, uint64 size) const;
    bool is_commit(const size_t page) const {
        SDL_ASSERT(page < max_commit_page);
        return m_commit[page];
    }
    void set_commit(const size_t page, const bool value) {
        SDL_ASSERT(page < max_commit_page);
        m_commit[page] = value;
    }
private:
    using commit_set = std::bitset<max_commit_page>;
    std::unique_ptr<char[]> m_base_address;
    commit_set m_commit;
};

} // mmu
} // sdl
} // db

#endif // __SDL_MEMORY_VM_MALLOC_H__