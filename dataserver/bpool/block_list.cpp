// block_list.cpp
//
#include "dataserver/bpool/block_list.h"
#include "dataserver/bpool/page_bpool.h"

#if defined(SDL_FOR_SAFETY)
#error SDL_FOR_SAFETY
#endif
#define SDL_FOR_SAFETY(...) ((void)0)

namespace sdl { namespace db { namespace bpool {

#if SDL_DEBUG
bool block_list_t::assert_list(const freelist f, const tracef t) const
{
    if (tracef::true_ == t) {
        std::cout << m_name << "(" << m_block_list << ") = ";
    }
    SDL_ASSERT(!m_block_list == !m_block_tail);
    auto p = m_block_list;
    while (p) {
        if (tracef::true_ == t) {
            std::cout << p << " ";
        }
        block_head const * const head = first_block_head(p, f);
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
    if (tracef::true_ == t) {
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
        SDL_FOR_SAFETY(item->nextBlock = null);
    }
    SDL_FOR_SAFETY(item->prevBlock = null);
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

bool block_list_t::remove(block_head * const item, block32 const blockId)
{
    SDL_ASSERT(blockId && item);
    SDL_ASSERT(item->blockId == blockId);
    if (empty()) {
        SDL_ASSERT(0);
        return false;
    }
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
        SDL_FOR_SAFETY(item->prevBlock = null);
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
    return true;
}

size_t block_list_t::truncate(block_list_t & dest, size_t const block_count)
{
    SDL_ASSERT(this != &dest);
    SDL_ASSERT(dest.empty());
    if (!block_count || empty()) {
        return 0;
    }
    size_t count = 1;
    block32 p_head = m_block_tail;
    block32 const p_tail = p_head;
    block_head * p = m_p->first_block_head(p_head);
    while (count < block_count) {
        if (p->prevBlock) {
            p = m_p->first_block_head(p_head = p->prevBlock);
            ++count;
        }
        else {
            break;
        }
    }
    SDL_ASSERT(p->blockId == p_head);
    if (p->prevBlock) {
        block_head * const tail = m_p->first_block_head(p->prevBlock);
        tail->nextBlock = null;
        m_block_tail = p->prevBlock;
        SDL_ASSERT(tail->blockId == m_block_tail);
        SDL_ASSERT(!empty());
        SDL_ASSERT(m_block_tail != p_tail);
    }
    else {
        SDL_ASSERT(p->blockId == m_block_list);
        m_block_list = m_block_tail = 0;
        SDL_ASSERT(empty());
    }
    SDL_ASSERT(p_head && p_tail);
    if (dest.empty()) {
        dest.m_block_list = p_head;
        dest.m_block_tail = p_tail;
        p->prevBlock = null;
    }
    else {
        block_head * const d = m_p->first_block_head(dest.m_block_tail);
        d->nextBlock = p_head;
        p->prevBlock = dest.m_block_tail;
        dest.m_block_tail = p_tail;
    }
    SDL_ASSERT_DEBUG_2(assert_list());
    SDL_ASSERT_DEBUG_2(dest.assert_list());
    return count;
}

bool block_list_t::append(block_list_t && src, freelist const f)
{
    SDL_ASSERT(this != &src);
    if (src.empty()) {
        SDL_ASSERT(0);
        return false;
    }
    if (empty()) {
        m_block_list = src.m_block_list;
        m_block_tail = src.m_block_tail;
    }
    else {
        SDL_ASSERT(m_block_tail);
        block_head * p = m_p->first_block_head(m_block_tail, f);
        SDL_ASSERT(!p->nextBlock);
        p->nextBlock = src.m_block_list;
        p = m_p->first_block_head(src.m_block_list, f);
        SDL_ASSERT(!p->prevBlock);
        p->prevBlock = m_block_tail;
        SDL_ASSERT(m_block_tail != src.m_block_tail);
        m_block_tail = src.m_block_tail;
    }
    src.m_block_list = null;
    src.m_block_tail = null;
    SDL_ASSERT(!empty() && src.empty());
    return true;
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
