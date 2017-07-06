// vm_win32.h
//
#pragma once
#ifndef __SDL_SYSTEM_VM_WIN32_H__
#define __SDL_SYSTEM_VM_WIN32_H__

#include "dataserver/system/page_head.h"
#include "dataserver/spatial/sparse_set.h"

#if defined(SDL_OS_WIN32)

#include <windows.h>

namespace sdl { namespace db {

class vm_base: noncopyable {
public:
    enum { block_page_num = 64 };                                   // 2^6   
    enum { page_size = page_head::page_size };                      // 8 KB = 8192 byte = 2^13
    enum { block_size = page_size * block_page_num };               // 512 KB = 524288 byte = 2^19
    static constexpr size_t max_page = size_t(1) << 32;             // 4,294,967,296 = 2^32
    static constexpr size_t max_block = max_page / block_page_num;  // 67,108,864 = 2^26
    static constexpr size_t max_block_bytes = max_block / 8;        // 8,388,608 = 2^23
};

class vm_win32 : vm_base {
public:
    size_t const byte_reserved;
    size_t const page_reserved;
    explicit vm_win32(size_t);
    ~vm_win32();
    char const * get() const {
        return m_base_address;
    }
    bool is_open() const {
        return get() != nullptr;
    }
    void * alloc(char const * start, size_t);
    void release(char const * start, size_t);
private:
    char const * end() const {
        return get() + page_reserved * page_size;
    }
    bool assert_address(char const * const start, size_t const size) const {
        SDL_ASSERT(start);
        SDL_ASSERT(size && !(size % page_size));
        SDL_ASSERT(get() <= start);
        SDL_ASSERT(start + size <= end());
        return true;
    }
private:
    char * m_base_address = nullptr;
    sparse_set<uint64> m_block_commit;
};

} // sdl
} // db

#endif // SDL_OS_WIN32
#endif // __SDL_SYSTEM_VM_WIN32_H__
