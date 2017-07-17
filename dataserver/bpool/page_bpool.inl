// page_bpool.inl
//
#pragma once
#ifndef __SDL_BPOOL_PAGE_BPOOL_INL__
#define __SDL_BPOOL_PAGE_BPOOL_INL__

namespace sdl { namespace db { namespace bpool {

inline bool page_bpool::is_open() const
{
    return m_file.is_open() && m_alloc.is_open();
}

inline void const * page_bpool::start_address() const
{
    return m_alloc.base_address();
}

inline size_t page_bpool::page_count() const
{
    return info.page_count;
}

inline block_head *
page_bpool::get_block_head(page_head * const p) {
    static_assert(sizeof(block_head) == page_head::reserved_size, "");
    uint8 * const b = p->data.reserved;
    return reinterpret_cast<block_head *>(b);
}

inline page_head const *
page_bpool::zero_block_page(pageIndex const pageId) {
    SDL_ASSERT(pageId.value() < pool_limits::block_page_num);
    char * const page_adr = m_alloc.base_address() + page_offset(pageId) * pool_limits::page_size;
    return reinterpret_cast<page_head *>(page_adr);
}

}}} // sdl

#endif // __SDL_BPOOL_PAGE_BPOOL_INL__
