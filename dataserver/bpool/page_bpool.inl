// page_bpool.inl
//
#pragma once
#ifndef __SDL_BPOOL_PAGE_BPOOL_INL__
#define __SDL_BPOOL_PAGE_BPOOL_INL__

namespace sdl { namespace db { namespace bpool {

inline bool page_bpool::is_open() const {
    return m_file.is_open() && m_alloc.is_open();
}

inline size_t page_bpool::page_count() const {
    return info.page_count;
}

inline bool page_bpool::is_zero_block(pageIndex const pageId) {
    SDL_ASSERT(pageId.value() < pool_limits::max_page);
    return pageId.value() < pool_limits::block_page_num;
}

inline uint32 page_bpool::realBlock(pageIndex const pageId) { // file block 
    SDL_ASSERT(pageId.value() < pool_limits::max_page);
    return pageId.value() / pool_limits::block_page_num;
}

inline page_head *
page_bpool::get_block_page(char * const block_adr, size_t const i) {
    SDL_ASSERT(block_adr);
    SDL_ASSERT(i < pool_limits::block_page_num);
    return reinterpret_cast<page_head *>(block_adr + i * pool_limits::page_size);
}

inline block_head const *
page_bpool::get_block_head(block32 const blockId, pageIndex const pageId) const {
    char const * const block_adr = m_alloc.get_block(blockId);
    char const * const page_adr = block_adr + page_head::page_size * page_bit(pageId);
    page_head const * const page = reinterpret_cast<page_head const *>(page_adr);
    return block_head::get_block_head(page);
}

inline block_head *
page_bpool::get_block_head(char * const block_adr, size_t const i) {
    return block_head::get_block_head(get_block_page(block_adr, i));
}

inline block_head *
page_bpool::first_block_head(char * const block_adr) {
    SDL_ASSERT(block_adr);
    return block_head::get_block_head(reinterpret_cast<page_head *>(block_adr));
}

inline block_head *
page_bpool::first_block_head(block32 const blockId) const {
    SDL_ASSERT(blockId);
    block_head * const p = first_block_head(m_alloc.get_block(blockId));
    SDL_ASSERT(p->blockId == blockId);
    return p;
}

inline block_head *
page_bpool::first_block_head(block32 const blockId, freelist const f) const {
    SDL_ASSERT(blockId);
    block_head * const p = first_block_head(m_alloc.get_block(blockId));
    SDL_ASSERT(p->blockId == blockId);
    SDL_ASSERT(!p->realBlock == (f == freelist::true_));
    return p;
}

inline pageIndex
page_bpool::block_pageIndex(pageIndex const pageId) {
    return pageId.value() - (pageId.value() % 8);
}

inline pageIndex
page_bpool::block_pageIndex(pageIndex const pageId, size_t const i) {
    SDL_ASSERT(i < pool_limits::block_page_num);
    return pageId.value() - (pageId.value() % 8) + static_cast<page32>(i);
}

inline page_head const *
page_bpool::zero_block_page(pageIndex const pageId) {
    SDL_ASSERT(pageId.value() < pool_limits::block_page_num);
    char * const page_adr = m_zero_block_address + page_bit(pageId) * pool_limits::page_size;
    return reinterpret_cast<page_head *>(page_adr);
}

inline void page_bpool::read_block_from_file(char * const block_adr, size_t const blockId) {
     m_file.read(block_adr, blockId * pool_limits::block_size, info.block_size_in_bytes(blockId)); 
}

inline uint32 page_bpool::pageAccessTime() const {
#if 0
    return unix_time();
#else
    return ++m_pageAccessTime;
#endif
}

//----------------------------------------------------------------

}}} // sdl

#endif // __SDL_BPOOL_PAGE_BPOOL_INL__
