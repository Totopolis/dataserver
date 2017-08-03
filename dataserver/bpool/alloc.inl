// alloc.inl
//
#pragma once
#ifndef __SDL_BPOOL_ALLOC_INL__
#define __SDL_BPOOL_ALLOC_INL__

namespace sdl { namespace db { namespace bpool {

inline page_bpool_alloc::block32
page_bpool_alloc::get_block_id(char const * const block_adr) const {
    SDL_ASSERT(block_adr >= m_alloc.base_address());
    SDL_ASSERT(block_adr < m_alloc_brk);
    const size_t size = block_adr - base();
    SDL_ASSERT(!(size % pool_limits::block_size));
    SDL_ASSERT((size / pool_limits::block_size) < pool_limits::max_block);
    return static_cast<block32>(size >> power_of<block_size>::value);
}

inline char * page_bpool_alloc::get_block(block32 const id) const {
    static_assert(power_of<block_size>::value == 16, "");
    char * const p = base() + (static_cast<size_t>(id) << power_of<block_size>::value);
    SDL_ASSERT(p >= m_alloc.base_address());
    SDL_ASSERT(p < m_alloc_brk);
    SDL_ASSERT(!m_decommit.find(id));
    return p;
}


}}} // sdl

#endif // __SDL_BPOOL_ALLOC_INL__
