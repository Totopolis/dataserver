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
        const size_t j = (i % index_block_num) >> power_of<mask_div>::value; // divide by mask_div
        const size_t k = (i & mask_hex);
        uint64 const & mask = (*p)[j];
        return (mask & (uint64(1) << k)) != 0;
    }
    return false;
}

inline void thread_mask_t::clr_block(size_t const i) {
#if 0 //SDL_DEBUG
    if (i == 62976) {
        SDL_TRACE("clr_block[", i, "] = ", std::this_thread::get_id());
    }
#endif
    SDL_ASSERT(i < m_block_count);
    mask_p & p = m_index[i / index_block_num];
    if (p) {
        const size_t j = (i % index_block_num) >> power_of<mask_div>::value; // divide by mask_div
        const size_t k = (i & mask_hex);
        uint64 & mask = (*p)[j];
        mask &= ~(uint64(1) << k);
    }
}

inline void thread_mask_t::set_block(size_t const i) {
#if 0 //SDL_DEBUG
    if (i == 62976) {
        SDL_TRACE("set_block[", i, "] = ", std::this_thread::get_id());
    }
#endif
    SDL_ASSERT(i < m_block_count);
    mask_p & p = m_index[i / index_block_num];
    if (!p) {
        reset_new(p); //FIXME: should be optimized (custom allocator)
    }
    const size_t j = (i % index_block_num) >> power_of<mask_div>::value; // divide by mask_div
    const size_t k = (i & mask_hex);
    uint64 & mask = (*p)[j];
    mask |= (uint64(1) << k);
}

template<class fun_type>
void thread_mask_t::for_each_block(fun_type && fun) const {
    size_t block_offset = 0;
    for (mask_p const & p : m_index) {
        if (p) {
            mask_t const & m = *p;
            size_t block_index = block_offset;
            for (const uint64 & mask : m) {
                if (mask) {
                    static_assert(chunk_block_num == 64, "");
                    size_t b = block_index;
                    for (size_t k = 0; k < chunk_block_num; ++k, ++b) {
                        if (mask & (uint64(1) << k)) {
                            fun(b);
                        }
                    }
                }
                block_index += chunk_block_num;
            }
            SDL_ASSERT(block_index == (block_offset + index_block_num));
        }
        block_offset += index_block_num;
    }
    SDL_ASSERT(block_offset >= m_block_count);
}

}}} // sdl

#endif // __SDL_BPOOL_THREAD_ID_INL__