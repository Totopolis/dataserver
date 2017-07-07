// vm_win32.h
//
#pragma once
#ifndef __SDL_SYSTEM_VM_WIN32_H__
#define __SDL_SYSTEM_VM_WIN32_H__

#include "dataserver/system/page_head.h"

#if defined(SDL_OS_WIN32)

namespace sdl { namespace db {

class vm_win32 : noncopyable {
#if SDL_DEBUG
public:
#endif
    enum { slot_page_num = 8 };
    enum { block_slot_num = 8 };
    enum { page_size = page_head::page_size };                      // 8 KB = 8192 byte = 2^13
    enum { slot_size = page_size * slot_page_num };                 // 64 KB = 65536 byte = 2^16
    enum { block_size = slot_size * block_slot_num };               // 512 KB = 524288 byte = 2^19
    static constexpr size_t max_page = size_t(1) << 32;             // 4,294,967,296 = 2^32
    static constexpr size_t max_slot = max_page / slot_page_num;    // 536,870,912
    static constexpr size_t max_block = max_slot / block_slot_num;  // 67,108,864 = 8,388,608 * 8
public:
    size_t const byte_reserved;
    size_t const page_reserved;
    size_t const slot_reserved;
    size_t const block_reserved;
public:
    explicit vm_win32(size_t);
    ~vm_win32();
    char const * base_address() const {
        return m_base_address;
    }
    char const * end_address() const {
        return m_base_address + page_reserved * page_size;
    }
    bool is_open() const {
        return m_base_address != nullptr;
    }
    char const * alloc(char const * start, size_t);
    void release(char const * start, size_t);
private:
    bool assert_address(char const * const start, size_t const size) const {
        SDL_ASSERT(start);
        SDL_ASSERT(size && !(size % page_size));
        SDL_ASSERT(m_base_address <= start);
        SDL_ASSERT(start + size <= end_address());
        return true;
    }
    static char const * init_vm_alloc(size_t);
private:
    char const * const m_base_address = nullptr;
    std::vector<bool> m_block_commit;
};

} // sdl
} // db

#endif // SDL_OS_WIN32
#endif // __SDL_SYSTEM_VM_WIN32_H__
