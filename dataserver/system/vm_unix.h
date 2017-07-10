// vm_unix.h
//
#pragma once
#ifndef __SDL_SYSTEM_VM_UNIX_H__
#define __SDL_SYSTEM_VM_UNIX_H__

#include "dataserver/system/vm_base.h"

#if 0 //defined(SDL_OS_UNIX)

#include <bitset>

namespace sdl { namespace db {

class vm_unix: noncopyable {
    using this_error = sdl_exception_t<vm_unix>;
public:
    enum { max_commit_page = 1 + uint16(-1) }; // 65536 pages
    enum { page_size = page_head::page_size }; // 8192 byte
    uint64 const byte_reserved;
    uint64 const page_reserved;
    explicit vm_unix(uint64);
    ~vm_unix();
    bool is_open() const {
        return m_base_address != nullptr;
    }
    void * alloc(uint64 start, uint64 size);
    bool clear(uint64 start, uint64 size);
private:
    void * base_address() const {
        SDL_ASSERT(m_base_address);
        return m_base_address;
    }
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
    void * m_base_address = nullptr;
    commit_set m_commit;
};

} // sdl
} // db

#endif // SDL_OS_UNIX
#endif // __SDL_SYSTEM_VM_UNIX_H__
