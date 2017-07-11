// page_pool.inl
//
#pragma once
#ifndef __SDL_SYSTEM_PAGE_POOL_INL__
#define __SDL_SYSTEM_PAGE_POOL_INL__

#if SDL_TEST_PAGE_POOL

namespace sdl { namespace db { namespace pp {

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

} // pp
} // db
} // sdl

#endif // SDL_TEST_PAGE_POOL
#endif // __SDL_SYSTEM_PAGE_POOL_INL__
