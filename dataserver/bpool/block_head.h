// block_head.h
//
#pragma once
#ifndef __SDL_BPOOL_BLOCK_HEAD_H__
#define __SDL_BPOOL_BLOCK_HEAD_H__

#include "dataserver/system/page_head.h"
#include "dataserver/common/array_enum.h"
#include "dataserver/spatial/interval_set.h"

namespace sdl { namespace db { namespace bpool {
namespace unit {
    struct threadIndex{};
}
typedef quantity<unit::threadIndex, size_t> threadIndex;

using page32 = pageFileID::page32;

struct pool_limits final : is_static {
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

struct block_index final {
    static constexpr uint32 blockIdMask  = 0x00FFFFFF;
    static constexpr uint32 pageLockMask = 0xFF000000;
    using block32 = uint32;
    static constexpr block32 invalid_block32 = block32(-1);
    struct data_type {
        unsigned int blockId : 24;      // 1 terabyte address space 
        unsigned int pageLock : 8;      // bitmask
    };
    union {
        data_type d;
        uint32 value;
    };
    block32 blockId() const { // or address
        return d.blockId;
    }
    uint8 pageLock() const {
        return d.pageLock;
    }
    void clr_blockId();
    void set_blockId(block32);
    bool is_lock_page(size_t) const;
    uint8 set_lock_page(size_t); // return old pageLock
    uint8 clr_lock_page(size_t); // return new pageLock
    void set_lock_page_all() {
        d.pageLock = 0xFF;
    }
    bool can_free_unused() const;
};   

inline constexpr size_t page_bit(pageIndex pageId) {
    return pageId.value() & 7; // = pageId.value() % 8;
}

class page_bpool;
struct block_head final { // 32 bytes
    using thread64 = uint64;
    thread64 pageLockThread;        // 64 threads mask
    uint32 prevBlock;
    uint32 nextBlock;
    uint32 realBlock;               // real MDF block
#if SDL_DEBUG
    unsigned int d_blockId : 24;    // used only for diagnostic
#else
    unsigned int reserve24 : 24;      
#endif
    unsigned int fixedBlock : 8;    // block is fixed in memory
    uint64 reserve64;
    bool is_lock_thread(size_t) const;
    void set_lock_thread(size_t);
    thread64 clr_lock_thread(size_t); // return new pageLockThread
    void set_zero() {
        memset_zero(*this);
    }
    void set_zero_fixed() {
        memset_zero(*this);
        fixedBlock = 1;
    }
    bool is_zero() const {
        return memcmp_zero(*this);
    }
    void set_fixed() {
        fixedBlock = 1;
    } 
    void clr_fixed() {
        fixedBlock = 0;
    } 
    bool is_fixed() const {
        return fixedBlock != 0;
    }
    static block_head * get_block_head(page_head *);
    static block_head const * get_block_head(page_head const *);
    static page_head * get_page_head(block_head *);
    static page_head const * get_page_head(block_head const *);
};

#pragma pack(pop)

using interval_block32 = interval_set<block_index::block32>;

}}} // sdl

#include "dataserver/bpool/block_head.inl"

#endif // __SDL_BPOOL_BLOCK_HEAD_H__
