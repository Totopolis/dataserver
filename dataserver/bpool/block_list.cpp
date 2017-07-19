// block_list.cpp
//
#include "dataserver/bpool/block_list.h"
#include "dataserver/bpool/page_bpool.h"
#include <set>

namespace sdl { namespace db { namespace bpool {

#if SDL_DEBUG
bool block_list_t::assert_list(const bool trace) const
{
    if (trace) {
        if (m_name && m_name[0]) {
            std::cout << m_name;
        }
        std::cout << "(" << m_block_list << ") = ";
    }
    SDL_ASSERT(!m_block_list == !m_block_tail);
    std::set<block32> check;
    auto p = m_block_list;
    while (p) {
        SDL_ASSERT(check.insert(p).second);
        if (trace) {
            std::cout << p << " ";
        }
        block_head const * const head = m_p->first_block_head(p);
        SDL_ASSERT(head->blockId == p);
        if (p == m_block_list) {
            SDL_ASSERT(!head->prevBlock);
        }
        else {
            SDL_ASSERT(head->prevBlock);
        }        
        p = head->nextBlock;
        if (!p) {
            SDL_ASSERT(head->blockId == m_block_tail);
        }
    }
    if (trace) {
        std::cout << std::endl;
    }
    return true;
}

bool block_list_t::find_block(block32 const blockId) const
{
    SDL_ASSERT(blockId);
    SDL_ASSERT_DEBUG_2(assert_list());
    auto p = m_block_list;
    while (p) {
        if (trace_enable) {
            std::cout << p << " ";
        }
        if (p == blockId) {
            return true;
        }
        p = m_p->first_block_head(p)->nextBlock;
    }
    SDL_ASSERT(m_block_tail != blockId);
    return false;
}

#endif // SDL_DEBUG

void block_list_t::insert(block_head * const item, block32 const blockId)
{
    SDL_ASSERT(blockId && item);
    SDL_ASSERT(item->blockId == blockId);
    SDL_ASSERT(!item->prevBlock);
    SDL_ASSERT(!item->nextBlock);
    SDL_ASSERT(!find_block(blockId));
    SDL_ASSERT(m_block_tail != blockId);
    if (m_block_list) {
        SDL_ASSERT(m_block_tail);
        block_head * const old = m_p->first_block_head(m_block_list);
        old->prevBlock = blockId;
        item->nextBlock = m_block_list;
    }
    else {
        SDL_ASSERT(!m_block_tail);
        m_block_tail = blockId;
        //item->nextBlock = null; // for safety
    }
    //item->prevBlock = null; // for safety
    m_block_list = blockId;
    SDL_ASSERT(!empty());
    SDL_ASSERT_DEBUG_2(assert_list());
}

bool block_list_t::promote(block_head * const item, block32 const blockId)
{
    SDL_ASSERT(blockId && item);
    SDL_ASSERT(item->blockId == blockId);
    SDL_ASSERT(m_block_list);
    SDL_ASSERT(find_block(blockId));
    SDL_ASSERT(m_block_tail);
    if (m_block_list != blockId) {
        SDL_ASSERT(item->prevBlock);
        SDL_ASSERT(item->prevBlock != blockId);
        block_head * p = m_p->first_block_head(item->prevBlock);
        p->nextBlock = item->nextBlock; // can be 0
        if (item->nextBlock) {
            SDL_ASSERT(m_block_tail != blockId);
            p = m_p->first_block_head(item->nextBlock);
            SDL_ASSERT(p->prevBlock == blockId);
            p->prevBlock = item->prevBlock;
        }
        else {
            SDL_ASSERT(m_block_tail == blockId);
            SDL_ASSERT(!p->nextBlock);
            m_block_tail = item->prevBlock;
        }
        p = m_p->first_block_head(m_block_list);
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

void block_list_t::remove(block_head * const item, block32 const blockId)
{
    SDL_ASSERT(blockId && item);
    SDL_ASSERT(item->blockId == blockId);
    SDL_ASSERT(m_block_list);
    SDL_ASSERT(find_block(blockId));
    SDL_ASSERT(m_block_tail);
    if (m_block_list == blockId) {
        SDL_ASSERT(!item->prevBlock);
        m_block_list = item->nextBlock; // can be 0
        if (m_block_list) {
            block_head * const p = m_p->first_block_head(m_block_list);
            p->prevBlock = null;
            SDL_ASSERT(m_block_tail != blockId);
            SDL_ASSERT(!empty());
        }
        else {
            SDL_ASSERT(m_block_tail == blockId);
            m_block_tail = null;
            SDL_ASSERT(empty());
        }
        //item->prevBlock = null; // for safety
        item->nextBlock = null;
    }
    else {
        SDL_ASSERT(item->prevBlock);
        block_head * p = m_p->first_block_head(item->prevBlock);
        p->nextBlock = item->nextBlock; // can be 0
        if (item->nextBlock) {
            SDL_ASSERT(m_block_tail != blockId);
            p = m_p->first_block_head(item->nextBlock);
            SDL_ASSERT(p->prevBlock == blockId);
            p->prevBlock = item->prevBlock;
            item->prevBlock = null;
            item->nextBlock = null;
        }
        else {
            SDL_ASSERT(m_block_tail == blockId);
            m_block_tail = item->prevBlock;
            SDL_ASSERT(m_block_tail != blockId);
            item->prevBlock = null;
            SDL_ASSERT(!item->nextBlock);
        }        
        SDL_ASSERT(!empty());
    }
    SDL_ASSERT_DEBUG_2(assert_list());
    SDL_ASSERT(!item->prevBlock);
    SDL_ASSERT(!item->nextBlock);
}

#if SDL_DEBUG
namespace {
    class unit_test {
    public:
        unit_test() {
        }
    };
    static unit_test s_test;
}
#endif //#if SDL_DEBUG
}}} // sdl
