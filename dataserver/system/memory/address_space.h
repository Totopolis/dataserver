// address_tbl.h
//
#pragma once
#ifndef __SDL_SYSTEM_MEMORY_ADDRESS_SPACE_H__
#define __SDL_SYSTEM_MEMORY_ADDRESS_SPACE_H__

#include "dataserver/system/page_head.h"
#include "dataserver/spatial/interval_set.h"

namespace sdl { namespace db { namespace mmu {

struct address_tbl final : noncopyable {
    enum { page_size = page_head::page_size }; // 8192 byte (8 kilobyte)
    enum { max_page = 1 << 16 }; // 65536 pages
    using pagetable = array_t<uint16, max_page>; // 8192 * 65536 = 536870912 byte address space
    static_assert(sizeof(pagetable) == 65536 * sizeof(uint16), ""); // 131072 byte
    static_assert(max_page - 1 == uint16(-1), "");
    const std::unique_ptr<pagetable> tbl;
    address_tbl(): tbl(new pagetable){
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

} // mmu
} // db
} // sdl

#endif // __SDL_SYSTEM_MEMORY_ADDRESS_SPACE_H__
