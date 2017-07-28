// block_head.inl
//
#pragma once
#ifndef __SDL_BPOOL_BLOCK_HEAD_INL__
#define __SDL_BPOOL_BLOCK_HEAD_INL__

namespace sdl { namespace db { namespace bpool {

//-----------------------------------------------------------------

inline void block_index::set_blockId(const block32 v) {
    SDL_ASSERT(v < pool_limits::max_block);
    d.blockId = v;
}
inline void block_index::clr_blockId() {
    SDL_ASSERT(d.blockId && "warning");
    d.blockId = 0;
}
inline bool block_index::is_lock_page(const size_t i) const {
    SDL_ASSERT(i < 8);
    return 0 != (d.pageLock & (unsigned int)(1 << i));
}    
inline uint8 block_index::set_lock_page(const size_t i) {
    SDL_ASSERT(i < 8);
    const uint8 old = d.pageLock;
    d.pageLock |= (unsigned int)(1 << i);
    return old;
}    
inline uint8 block_index::clr_lock_page(const size_t i) {
    SDL_ASSERT(i < 8);
    d.pageLock &= ~(unsigned int)(1 << i);
    return d.pageLock;
} 
inline bool block_index::can_free_unused() const { // block is loaded and not locked
    return !pageLock() && blockId(); //return value && (value == (value & blockIdMask));
}
//-----------------------------------------------------------------

inline bool block_head::is_lock_thread(size_t const i) const {
    return bitmask<thread64>::is_bit(pageLockThread, i);
}
inline void block_head::set_lock_thread(size_t const i) {
    bitmask<thread64>::set_bit(pageLockThread, i);
}
inline block_head::thread64
block_head::clr_lock_thread(size_t const i) {
    bitmask<thread64>::clr_bit(pageLockThread, i);
    return pageLockThread;
}

//-----------------------------------------------------------------

inline page_head *
block_head::get_page_head(block_head * const block_adr) {
    SDL_ASSERT(block_adr);
    enum { offset = offsetof(page_head, data.reserved) };
    static_assert(offset == 0x40, "");
    char * const p = reinterpret_cast<char *>(block_adr) - offset;
    return reinterpret_cast<page_head *>(p);
}

inline page_head const *
block_head::get_page_head(block_head const * const block_adr) {
    SDL_ASSERT(block_adr);
    enum { offset = offsetof(page_head, data.reserved) };
    static_assert(offset == 0x40, "");
    char const * const p = reinterpret_cast<char const *>(block_adr) - offset;
    return reinterpret_cast<page_head const *>(p);
}

inline block_head *
block_head::get_block_head(page_head * const p) {
    static_assert(sizeof(block_head) == page_head::reserved_size, "");
    SDL_ASSERT(p);
    uint8 * const b = p->data.reserved;
    return reinterpret_cast<block_head *>(b);
}

inline block_head const *
block_head::get_block_head(page_head const * const p) {
    static_assert(sizeof(block_head) == page_head::reserved_size, "");
    SDL_ASSERT(p);
    uint8 const * const b = p->data.reserved;
    return reinterpret_cast<block_head const *>(b);
}


}}} // sdl

#endif // __SDL_BPOOL_BLOCK_HEAD_INL__
