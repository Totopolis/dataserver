// page_pool.inl
//
#pragma once
#ifndef __SDL_SYSTEM_PAGE_POOL_INL__
#define __SDL_SYSTEM_PAGE_POOL_INL__

#if !SDL_TEST_PAGE_POOL
//#error SDL_TEST_PAGE_POOL
#endif

namespace sdl { namespace db { namespace pp {

#if SDL_PAGE_POOL_MAX_MEM
inline bool PagePool::block_t::use_page(size_t const i) const {
    SDL_ASSERT(i < 64);
    return (pagemask & (uint64(1) << i)) != 0;
}

inline void PagePool::block_t::set_page(size_t const i, const bool f){
    SDL_ASSERT(i < 64);
    if (f) {
        pagemask |= (uint64(1) << i);
    }
    else {
        pagemask &= ~(uint64(1) << i);
    }
}
#endif

} // pp
} // db
} // sdl

#endif // __SDL_SYSTEM_PAGE_POOL_INL__
