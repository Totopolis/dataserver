// block_head.h
//
#pragma once
#ifndef __SDL_BPOOL_BLOCK_HEAD_H__
#define __SDL_BPOOL_BLOCK_HEAD_H__

#include "dataserver/system/page_head.h"
#include "dataserver/common/array_enum.h"

#if !defined(SDL_USE_BPOOL)
#error SDL_USE_BPOOL
#endif

namespace sdl { namespace db { namespace bpool {

using page32 = pageFileID::page32;

struct pool_limits {
    enum { max_thread = 64 };                                       // 64-bit mask
    enum { block_page_num = 8 };                                    // 1 extent
    enum { page_size = page_head::page_size };                      // 8 KB = 8192 byte = 2^13
    enum { block_size = page_size * block_page_num };               // 64 KB = 65536 byte = 2^16
    static constexpr size_t max_block = size_t(1) << 24;            // 2^24 = 16,777,216 => 16777216 * 64 KB = 2^40 (1 terabyte)
    static constexpr size_t max_page = max_block * block_page_num;  // 2^27 = 134,217,728
    static constexpr size_t last_block = max_block - 1;
    static constexpr size_t max_filesize = terabyte<1>::value;
};

#pragma pack(push, 1) 

struct block_index {
    struct data_type {
        unsigned int offset : 24;    // 1 terabyte address space 
        unsigned int pages : 8;     // bitmask (locked pages)
    };
    union {
        data_type d;
        uint32 value;
    };
    uint32 offset() const {
        return d.offset;
    }
    void set_offset(uint32);
    bool page(size_t) const;
    void set_page(size_t);
    void clear_page(size_t);
};   

struct block_head {    // 32 bytes
    uint32 prevBlock;
    uint32 nextBlock;
    uint32 blockId;
    uint64 pageLockThread;      // 64 threads
    uint32 pageAccessTime;
    uint32 pageAccessFreq;
    uint8 reserved[4];
    using threadMask = bitmask<uint64>;
};

#pragma pack(pop)

inline void block_index::set_offset(const uint32 v) {
    SDL_ASSERT(v < pool_limits::max_block);
    d.offset = v;
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

//-----------------------------------------------------------------
#if 0
namespace unit { struct threadIndex{}; }
typedef quantity<unit::threadIndex, size_t> threadIndex;
#endif
//-----------------------------------------------------------------

}}} // sdl

#endif // __SDL_BPOOL_BLOCK_HEAD_H__
