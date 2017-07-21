// block_list.inl
//
#pragma once
#ifndef __SDL_BPOOL_BLOCK_LIST_INL__
#define __SDL_BPOOL_BLOCK_LIST_INL__

namespace sdl { namespace db { namespace bpool {

inline void block_list_t::insert(block_head * const item, block32 const blockId)
{
    SDL_ASSERT(blockId && item);
    SDL_ASSERT(item->blockId == blockId);
    SDL_ASSERT(!item->prevBlock);
    SDL_ASSERT(!item->nextBlock);
    SDL_ASSERT(!find_block(blockId));
    SDL_ASSERT(m_block_tail != blockId);
    if (m_block_list) {
        SDL_ASSERT(m_block_tail);
        block_head * const old = m_p.first_block_head(m_block_list);
        old->prevBlock = blockId;
        item->nextBlock = m_block_list;
    }
    else {
        SDL_ASSERT(!m_block_tail);
        m_block_tail = blockId;
        SDL_ASSERT(item->nextBlock == null);
    }
    SDL_ASSERT(item->prevBlock == null);
    m_block_list = blockId;
    SDL_ASSERT(!empty());
    SDL_ASSERT_DEBUG_2(assert_list());
}

inline bool block_list_t::promote(block_head * const item, block32 const blockId)
{
    SDL_ASSERT(blockId && item);
    SDL_ASSERT(item->blockId == blockId);
    SDL_ASSERT(m_block_list);
    SDL_ASSERT(find_block(blockId));
    SDL_ASSERT(m_block_tail);
    if (m_block_list != blockId) {
        SDL_ASSERT(item->prevBlock);
        SDL_ASSERT(item->prevBlock != blockId);
        block_head * p = m_p.first_block_head(item->prevBlock);
        p->nextBlock = item->nextBlock; // can be 0
        if (item->nextBlock) {
            SDL_ASSERT(m_block_tail != blockId);
            p = m_p.first_block_head(item->nextBlock);
            SDL_ASSERT(p->prevBlock == blockId);
            p->prevBlock = item->prevBlock;
        }
        else {
            SDL_ASSERT(m_block_tail == blockId);
            SDL_ASSERT(!p->nextBlock);
            m_block_tail = item->prevBlock;
        }
        p = m_p.first_block_head(m_block_list);
        SDL_ASSERT(!p->prevBlock);
        p->prevBlock = blockId;
        item->prevBlock = 0;
        item->nextBlock = m_block_list;
        m_block_list = blockId;
        SDL_ASSERT_DEBUG_2(assert_list());
        return true;
    }
    return false;
}

}}} // sdl

#endif // __SDL_BPOOL_BLOCK_LIST_INL__