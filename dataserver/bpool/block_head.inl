// block_head.inl
//
#pragma once
#ifndef __SDL_BPOOL_BLOCK_HEAD_INL__
#define __SDL_BPOOL_BLOCK_HEAD_INL__

namespace sdl { namespace db { namespace bpool {

//-----------------------------------------------------------------

inline void block_index::set_blockId(const size_t v) {
    SDL_ASSERT(v < pool_limits::max_block);
    d.blockId = v;
}
inline bool block_index::is_lock_page(const size_t i) const {
    SDL_ASSERT(i < 8);
    return 0 != (d.pageLock & (unsigned int)(1 << i));
}    
inline void block_index::set_lock_page(const size_t i) {
    SDL_ASSERT(i < 8);
    d.pageLock |= (unsigned int)(1 << i);
}    
inline void block_index::clr_lock_page(const size_t i) {
    SDL_ASSERT(i < 8);
    d.pageLock &= ~(unsigned int)(1 << i);
} 

//-----------------------------------------------------------------

inline bool block_head::is_lock_thread(size_t const i) const {
    return bitmask<uint64>::is_bit(pageLockThread, i);
}
inline void block_head::set_lock_thread(size_t const i) {
    bitmask<uint64>::set_bit(pageLockThread, i);
}
inline void block_head::clr_lock_thread(size_t const i) {
    bitmask<uint64>::clr_bit(pageLockThread, i);
}

//-----------------------------------------------------------------

}}} // sdl

#endif // __SDL_BPOOL_BLOCK_HEAD_INL__
