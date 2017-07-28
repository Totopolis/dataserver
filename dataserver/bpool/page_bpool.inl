// page_bpool.inl
//
#pragma once
#ifndef __SDL_BPOOL_PAGE_BPOOL_INL__
#define __SDL_BPOOL_PAGE_BPOOL_INL__

namespace sdl { namespace db { namespace bpool {

inline bool page_bpool::is_open() const {
    return m_file.is_open() && m_alloc.base();
}

inline void const * page_bpool::start_address() const {
    return m_alloc.base();
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
page_bpool::get_page_head(block_head * const block_adr) {
    SDL_ASSERT(block_adr);
    enum { offset = offsetof(page_head, data.reserved) };
    static_assert(offset == 0x40, "");
    char * const p = reinterpret_cast<char *>(block_adr) - offset;
    return reinterpret_cast<page_head *>(p);
}

inline page_head *
page_bpool::get_block_page(char * const block_adr, size_t const i) {
    SDL_ASSERT(block_adr);
    SDL_ASSERT(i < pool_limits::block_page_num);
    return reinterpret_cast<page_head *>(block_adr + i * pool_limits::page_size);
}

inline block_head *
page_bpool::get_block_head(page_head * const p) {
    static_assert(sizeof(block_head) == page_head::reserved_size, "");
    SDL_ASSERT(p);
    uint8 * const b = p->data.reserved;
    return reinterpret_cast<block_head *>(b);
}

inline block_head const *
page_bpool::get_block_head(page_head const * const p) {
    static_assert(sizeof(block_head) == page_head::reserved_size, "");
    SDL_ASSERT(p);
    uint8 const * const b = p->data.reserved;
    return reinterpret_cast<block_head const *>(b);
}

inline block_head *
page_bpool::get_block_head(char * const block_adr, size_t const i) {
    return get_block_head(get_block_page(block_adr, i));
}

inline block_head *
page_bpool::first_block_head(char * const block_adr) {
    SDL_ASSERT(block_adr);
    return get_block_head(reinterpret_cast<page_head *>(block_adr));
}

inline block_head *
page_bpool::first_block_head(block32 const blockId) const {
    SDL_ASSERT(blockId);
    block_head * const p = first_block_head(m_alloc.get_block(blockId));
    SDL_ASSERT(p->blockId == blockId);
    SDL_ASSERT(p->realBlock);
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
    char * const page_adr = m_alloc.base() + page_bit(pageId) * pool_limits::page_size;
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

inline bool page_bpool::unlock_page(page_head const * const p) {
    if (p) {
        return unlock_page(p->data.pageId.pageId);
    }
    return false;
}

//----------------------------------------------------------------

inline block_head *
page_bpool_friend::first_block_head(block32 const blockId) const {
    return m_p->first_block_head(blockId);
}

inline block_head *
page_bpool_friend::first_block_head(block32 const blockId, freelist const f) const {
    return m_p->first_block_head(blockId, f);
}

}}} // sdl

#endif // __SDL_BPOOL_PAGE_BPOOL_INL__
