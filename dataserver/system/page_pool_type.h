// page_pool_type.h
//
#pragma once
#ifndef __SDL_SYSTEM_PAGE_POOL_TYPE_H__
#define __SDL_SYSTEM_PAGE_POOL_TYPE_H__

#include "dataserver/system/page_head.h"

#if !defined(SDL_TEST_PAGE_POOL)
#error SDL_TEST_PAGE_POOL
#endif

namespace sdl { namespace db { namespace bpool {

struct pool_limits {
    enum { block_page_num = 8 };                                    // 1 extent
    enum { page_size = page_head::page_size };                      // 8 KB = 8192 byte = 2^13
    enum { block_size = page_size * block_page_num };               // 64 KB = 65536 byte = 2^16
    static constexpr size_t max_page = size_t(1) << 32;             // 4,294,967,296 = 2^32
    static constexpr size_t max_block = max_page / block_page_num;  // 536,870,912 = 2^29
    static constexpr size_t max_bindex = size_t(1) << 24;
    static constexpr size_t last_bindex = max_bindex - 1;
};

#pragma pack(push, 1) 

struct block_head {
    struct data_type {
        unsigned int index : 24;    // 1024 GB address space 
        unsigned int pages : 8;     // page mask (lock pages in memory)
    };
    union {
        data_type d;
        uint32 value;
    };
    bool page(const size_t i) const {
        SDL_ASSERT(i < 8);
        return 0 != (d.pages & (unsigned int)(1 << i));
    }    
    void set_page(const size_t i) {
        SDL_ASSERT(i < 8);
        d.pages |= (unsigned int)(1 << i);
    }    
    void clear_page(const size_t i) {
        SDL_ASSERT(i < 8);
        d.pages &= ~(unsigned int)(1 << i);
    }    
};   

#pragma pack(pop)

using block_head_vector = std::vector<block_head>;

}}} // sdl

#endif // __SDL_SYSTEM_PAGE_POOL_TYPE_H__
