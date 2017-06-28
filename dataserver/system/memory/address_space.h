// address_space.h
//
#pragma once
#ifndef __SDL_SYSTEM_MEMORY_ADDRESS_SPACE_H__
#define __SDL_SYSTEM_MEMORY_ADDRESS_SPACE_H__

#include "dataserver/system/page_head.h"

namespace sdl { namespace db { namespace mmu {

class address_translation : noncopyable {
    enum { page_size = page_head::page_size }; // 8192 byte (8 kilobyte)
    enum { max_page = 1 << 16 }; // 65536 pages
    using pagetable = array_t<uint16, max_page>; // 8192 * 65536 = 536870912 bytes address space
    static_assert(sizeof(pagetable) == 65536 * sizeof(uint16), ""); // 131072 bytes
    static_assert(max_page - 1 == uint16(-1), "");
    const std::unique_ptr<pagetable> tbl;
public:
    address_translation() : tbl(new pagetable){
        tbl->fill_0();
    }
    static constexpr size_t size() { return max_page; }
    uint16 operator[](size_t const i) const noexcept {
        SDL_ASSERT(i < size());
        return (*tbl)[i];
    }
    uint16 & page(size_t const i) noexcept {
        SDL_ASSERT(i < size());
        return (*tbl)[i];
    }
};

class vm_alloc : noncopyable {
    enum { page_size = page_head::page_size }; // 8192 byte
public:
    explicit vm_alloc(uint64);
    ~vm_alloc();
    uint64 reserved() const { return m_reserved; }
    void * alloc(uint64 start, uint64 size);
private:
    uint64 const m_reserved;
};

} // mmu
} // db
} // sdl

#endif // __SDL_SYSTEM_MEMORY_ADDRESS_SPACE_H__
