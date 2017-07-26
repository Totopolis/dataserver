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

inline size_t page_bit(pageIndex pageId) {
    return pageId.value() & 7; // = pageId.value() % 8;
}

class page_bpool;
struct block_head final {       // 32 bytes
    using thread64 = uint64;
    thread64 pageLockThread;    // 64 threads mask
    //uint32 pageAccessTime;
#if SDL_DEBUG
    uint32 prevBlock;
    uint32 nextBlock;
    uint32 realBlock;           // real MDF block
    uint32 blockId;             // diagnostic
    page_bpool * bpool;         // for lock_page_head
#else
    uint32 prevBlock;
    uint32 nextBlock;
    uint32 realBlock;           // real MDF block
    uint32 reserve32;
    page_bpool * bpool;         // for lock_page_head
#endif
    bool is_lock_thread(size_t) const;
    void set_lock_thread(size_t);
    thread64 clr_lock_thread(size_t); // return new pageLockThread
    void set_zero() {
        memset_zero(*this);
    }
    bool is_zero() const {
        return memcmp_zero(*this);
    }
};

#pragma pack(pop)

class lock_page_head : noncopyable {
    friend class page_bpool;
    page_head const * m_p = nullptr;
    lock_page_head(page_head const * const p) noexcept : m_p(p) {}
public:
    lock_page_head() = default;
    lock_page_head(lock_page_head && src) noexcept
        : m_p(src.m_p) { //note: thread owner must be the same
        src.m_p = nullptr;
    }
    ~lock_page_head(); // see page_bpool.cpp
    void swap(lock_page_head & src) noexcept {
        std::swap(m_p, src.m_p); //note: thread owner must be the same
    }
    lock_page_head & operator = (lock_page_head && src) {
        lock_page_head(std::move(src)).swap(*this);
        SDL_ASSERT(!src);
        return *this;
    }
    page_head const * get() const {
        return m_p;
    }
    explicit operator bool() const {
        return m_p != nullptr;
    }
    void reset() {
        lock_page_head().swap(*this);
    }
};

}}} // sdl

#include "dataserver/bpool/block_head.inl"

#endif // __SDL_BPOOL_BLOCK_HEAD_H__
