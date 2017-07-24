// thread_id.inl
//
#pragma once
#ifndef __SDL_BPOOL_THREAD_ID_INL__
#define __SDL_BPOOL_THREAD_ID_INL__

namespace sdl { namespace db { namespace bpool { 

inline bool thread_mask_t::is_block(size_t const i) const {
    SDL_ASSERT(i < m_block_count);
    mask_p const & p = m_index[i / index_block_num];
    if (p) {
        const size_t j = (i % index_block_num) / mask_div;
        const size_t k = (i & mask_hex);
        uint64 const & mask = (*p)[j];
        return (mask & (uint64(1) << k)) != 0;
    }
    return false;
}

inline void thread_mask_t::clr_block(size_t const i) {
    SDL_ASSERT(i < m_block_count);
    mask_p & p = m_index[i / index_block_num];
    if (p) {
        const size_t j = (i % index_block_num) / mask_div;
        const size_t k = (i & mask_hex);
        uint64 & mask = (*p)[j];
        mask &= ~(uint64(1) << k);
    }
}

inline void thread_mask_t::set_block(size_t const i) {
    SDL_ASSERT(i < m_block_count);
    mask_p & p = m_index[i / index_block_num];
    if (!p) {
        reset_new(p);
    }
    const size_t j = (i % index_block_num) / mask_div;
    const size_t k = (i & mask_hex);
    uint64 & mask = (*p)[j];
    mask |= (uint64(1) << k);
}

}}} // sdl

#endif // __SDL_BPOOL_THREAD_ID_INL__