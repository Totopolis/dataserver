// block_head.h
//
#pragma once
#ifndef __SDL_BPOOL_BLOCK_HEAD_H__
#define __SDL_BPOOL_BLOCK_HEAD_H__

#include "dataserver/system/page_head.h"

#if !defined(SDL_USE_BPOOL)
#error SDL_USE_BPOOL
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

struct block_index {
    struct data_type {
        unsigned int index : 24;    // 1024 GB address space 
        unsigned int pages : 8;     // bitmask (used pages)
    };
    union {
        data_type d;
        uint32 value;
    };
    uint32 index() const {
        return d.index;
    }
    void set_index(uint32);
    bool page(size_t) const;
    void set_page(size_t);
    void clear_page(size_t);
};   

struct block_head {    // 32 bytes
    uint32 prevBlock;
    uint32 nextBlock;
    uint32 blockId;         // diagnostic
    uint64 pageLockMask;    // 64 threads
    uint32 pageAccessTime;
    uint32 pageAccessFreq;
    uint8 reserved[4];
};

#pragma pack(pop)

inline void block_index::set_index(const uint32 v) {
    SDL_ASSERT(v < pool_limits::max_bindex);
    d.index = v;
}
inline bool block_index::page(const size_t i) const {
    SDL_ASSERT(i < 8);
    return 0 != (d.pages & (unsigned int)(1 << i));
}    
inline void block_index::set_page(const size_t i) {
    SDL_ASSERT(i < 8);
    d.pages |= (unsigned int)(1 << i);
}    
inline void block_index::clear_page(const size_t i) {
    SDL_ASSERT(i < 8);
    d.pages &= ~(unsigned int)(1 << i);
} 

}}} // sdl

#endif // __SDL_BPOOL_BLOCK_HEAD_H__
