// block_list.cpp
//
#include "dataserver/bpool/block_list.h"
#include "dataserver/bpool/page_bpool.h"

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
        block_head const * const head = m_p.first_block_head(p, f);
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

void block_list_t::trace(freelist const f) const
{
    if (!empty()) {
        using set_type = interval_set<block32>;
        set_type set;
        this->for_each([&set](block_head *, block32 const p){
            set.insert(p);
            return true;
        }, f);
        SDL_ASSERT(!set.empty());
        set.trace();
    }
}

#endif // SDL_DEBUG

size_t block_list_t::length() const // O(N)
{
    size_t count = 0;
    auto p = m_block_list;
    while (p) {
        ++count;
        p = m_p.first_block_head(p)->nextBlock;
    }
    return count;
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
        p = m_p.first_block_head(p)->nextBlock;
    }
    SDL_ASSERT(m_block_tail != blockId);
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
            block_head * const p = m_p.first_block_head(m_block_list);
            p->prevBlock = null;
            SDL_ASSERT(m_block_tail != blockId);
            SDL_ASSERT(!empty());
        }
        else {
            SDL_ASSERT(m_block_tail == blockId);
            m_block_tail = null;
            SDL_ASSERT(empty());
        }
        SDL_ASSERT(item->prevBlock == null);
        item->nextBlock = null;
    }
    else {
        SDL_ASSERT(item->prevBlock);
        block_head * p = m_p.first_block_head(item->prevBlock);
        p->nextBlock = item->nextBlock; // can be 0
        if (item->nextBlock) {
            SDL_ASSERT(m_block_tail != blockId);
            p = m_p.first_block_head(item->nextBlock);
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
    block_head * p = m_p.first_block_head(p_head);
    while (count < block_count) {
        if (p->prevBlock) {
            p = m_p.first_block_head(p_head = p->prevBlock);
            ++count;
        }
        else {
            break;
        }
    }
    SDL_ASSERT(p->blockId == p_head);
    if (p->prevBlock) {
        block_head * const tail = m_p.first_block_head(p->prevBlock);
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
        block_head * const d = m_p.first_block_head(dest.m_block_tail);
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
        block_head * p = m_p.first_block_head(m_block_tail, f);
        SDL_ASSERT(!p->nextBlock);
        p->nextBlock = src.m_block_list;
        p = m_p.first_block_head(src.m_block_list, f);
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

block_list_t::block_head_Id
block_list_t::pop_head(freelist const f) {
    SDL_ASSERT(!empty());
    block32 const blockId = m_block_list;
    block_head * const p = m_p.first_block_head(blockId, f);
    SDL_ASSERT(!p->prevBlock);
    if (p->nextBlock) {
        block_head * const next = m_p.first_block_head(p->nextBlock, f);
        SDL_ASSERT(next->prevBlock == blockId);
        next->prevBlock = null;
        m_block_list = p->nextBlock;
        p->nextBlock = null;
        SDL_ASSERT(!empty());
    }
    else {
        SDL_ASSERT(m_block_list == m_block_tail);
        SDL_ASSERT(m_block_list == p->blockId);
        m_block_list = m_block_tail = null;
    }
    return { p, blockId };
}

void block_list_t::for_each_insert(interval_block32 & dest, freelist const f) const {
    for_each([&dest](block_head *, block32 p){
        dest.insert(p);
        return bc::continue_;
    }, f);
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
