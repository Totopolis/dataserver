// vm_unix.inl
//
#pragma once
#ifndef __SDL_BPOOL_VM_UNIX_INL__
#define __SDL_BPOOL_VM_UNIX_INL__

namespace sdl { namespace db { namespace bpool { 

inline void vm_unix_new::alloc_arena(arena_t & x
#if SDL_DEBUG
    , const size_t index
#endif
) {
    SDL_TRACE_DEBUG_2("alloc_arena[", index, "]"); 
    if (!x.arena_adr) {
        x.arena_adr = sys_alloc_arena();
        SDL_ASSERT(debug_zero_arena(x));
    }
    SDL_ASSERT(x.arena_adr && !x.block_mask);
}

inline void vm_unix_new::free_arena(arena_t & x
#if SDL_DEBUG
    , const size_t index
#endif
) {
    SDL_TRACE_DEBUG_2("free_arena[", index, "]"); 
    SDL_ASSERT(x.arena_adr && x.empty());
    sys_free_arena(x.arena_adr);
    x.arena_adr = nullptr;
}

//----------------------------------------------

inline void vm_unix_new::arena_t::zero_arena(){
    SDL_ASSERT(arena_adr);
    memset(arena_adr, 0, arena_size);
}
inline bool vm_unix_new::arena_t::is_block(size_t const i) const {
    SDL_ASSERT(arena_adr);
    SDL_ASSERT(i < arena_block_num);
    return 0 != (block_mask & (mask16)(1 << i));
}
template<size_t i>
inline bool vm_unix_new::arena_t::is_block() const {
    SDL_ASSERT(arena_adr);
    static_assert(i < arena_block_num, "");
    return 0 != (block_mask & (mask16)(1 << i));
}
inline void vm_unix_new::arena_t::set_block(size_t const i) {
    SDL_ASSERT(arena_adr);
    SDL_ASSERT(i < arena_block_num);
    SDL_ASSERT(!is_block(i));
    block_mask |= (mask16)(1 << i);
}
template<size_t i>
void vm_unix_new::arena_t::set_block() {
    SDL_ASSERT(arena_adr);
    static_assert(i < arena_block_num, "");
    SDL_ASSERT(!is_block<i>());
    block_mask |= (mask16)(1 << i);
}
inline void vm_unix_new::arena_t::clr_block(size_t const i) {
    SDL_ASSERT(arena_adr);
    SDL_ASSERT(i < arena_block_num);
    SDL_ASSERT(is_block(i));
    block_mask &= ~(mask16(1 << i));
}
inline bool vm_unix_new::arena_t::full() const {
    SDL_ASSERT(!block_mask || arena_adr);
    return arena_t::mask_all == block_mask;
}
inline bool vm_unix_new::arena_t::empty() const {
    SDL_ASSERT(!block_mask || arena_adr);
    return !block_mask;
}
inline bool vm_unix_new::arena_t::mixed() const {
    return !(empty() || full());
}

//http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetKernighan
//Counting bits set, Brian Kernighan's way
inline size_t vm_unix_new::arena_t::set_block_count() const {
    size_t count = 0;
    mask16 b = block_mask;
    while (b) {
        b &= b - 1; // clear the least significant bit set
        ++count;
    }
    SDL_ASSERT(count <= arena_block_num);
    return count;
}

inline size_t vm_unix_new::arena_t::free_block_count() const {
#if 0
    size_t count = 0;
    mask16 b = block_mask;
    while (b) {
        if (!(b & 1)) {
            ++count;
        }
        b >>= 1;
    }
    SDL_ASSERT(count <= arena_block_num);
    SDL_DEBUG_CPP(size_t set_count = set_block_count());
    SDL_ASSERT(count == (arena_block_num - set_count));
    return count;
#else
    return arena_block_num - set_block_count();
#endif
}
inline size_t vm_unix_new::arena_t::find_free_block() const {
    SDL_ASSERT(!full());
    size_t index = 0;
    mask16 b = block_mask;
    while (b & 1) {
        ++index;
        b >>= 1;
    }
    SDL_ASSERT(index < arena_block_num);
    return index;
}

}}} // db

#endif // __SDL_BPOOL_VM_UNIX_INL__
