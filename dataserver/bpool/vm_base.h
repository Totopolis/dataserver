// vm_base.h
//
#pragma once
#ifndef __SDL_BPOOL_VM_BASE_H__
#define __SDL_BPOOL_VM_BASE_H__

#include "dataserver/system/page_head.h"

namespace sdl { namespace db { namespace bpool {

enum class vm_commited { false_, true_ };

inline constexpr bool is_commited(vm_commited v) {
    return v != vm_commited::false_;
}

class vm_base : noncopyable {
public:
    enum { slot_page_num = 8 };
    enum { block_slot_num = 8 };
    enum { block_page_num = block_slot_num * slot_page_num };       // 64 pages
    enum { page_size = page_head::page_size };                      // 8 KB = 8192 byte = 2^13
    enum { slot_size = page_size * slot_page_num };                 // 64 KB = 65536 byte = 2^16
    enum { block_size = slot_size * block_slot_num };               // 512 KB = 524288 byte = 2^19
    static constexpr size_t max_page = size_t(1) << 32;             // 4,294,967,296 = 2^32
    static constexpr size_t max_slot = max_page / slot_page_num;    // 536,870,912 = 2^29
    static constexpr size_t max_block = max_slot / block_slot_num;  // 67,108,864 = 8,388,608 * 8 = 2^26
};

#if defined(SDL_OS_WIN32)
class vm_test : public vm_base {
public:
    enum { page_size = page_head::page_size };  
    size_t const byte_reserved;
    size_t const page_reserved;
    vm_test(size_t const size, vm_commited)
        : byte_reserved(size)
        , page_reserved(size / page_size) {
        SDL_ASSERT(size && !(size % page_size));
        m_base_address.reset(new char[size]);
    }
    char * base_address() const {
        return m_base_address.get();
    }
    char * end_address() const {
        return base_address() + byte_reserved;
    }
    bool is_open() const {
        return base_address() != nullptr;
    }
    char * alloc(char * const start, size_t const size) { 
        SDL_ASSERT(assert_address(start, size));
        return start;
    }
    char * alloc_all() {
        return alloc(base_address(), byte_reserved);
    }
    bool release(char * const start, size_t const size) { 
        SDL_ASSERT(assert_address(start, size));
        return true;
    }
    bool release_all() { 
        return release(base_address(), byte_reserved);
    }
private:
    bool assert_address(char const * const start, size_t const size) const {
        SDL_ASSERT(start);
        SDL_ASSERT(size && !(size % page_size));
        SDL_ASSERT(base_address() <= start);
        SDL_ASSERT(start + size <= end_address());
        return true;
    }
    std::unique_ptr<char[]> m_base_address; // huge memory
};
#endif // SDL_DEBUG

}}} // db

#endif // __SDL_BPOOL_VM_BASE_H__
