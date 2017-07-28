// block_list.h
//
#pragma once
#ifndef __SDL_BPOOL_BLOCK_LIST_H__
#define __SDL_BPOOL_BLOCK_LIST_H__

#include "dataserver/bpool/block_head.h"

namespace sdl { namespace db { namespace bpool {

enum class freelist { false_, true_ };
enum class tracef { false_, true_ };

class page_bpool;
class page_bpool_friend { // copyable
    using block32 = block_index::block32;
    page_bpool const * m_p;
public:
    page_bpool_friend(page_bpool const * p): m_p(p) { // allow implicit conversion
        SDL_ASSERT(m_p);
    }
    block_head * first_block_head(block32) const;
    block_head * first_block_head(block32, freelist) const;
};

class block_list_t : noncopyable {
    using block32 = block_index::block32;
public:
    enum { null = 0 };
    block_list_t(page_bpool_friend && p, const char * const s = "")
        : m_p(p), m_name(s) {
        SDL_ASSERT(m_name);
    }
    block32 head() const {
        return m_block_list;
    }
    block32 tail() const {
        return m_block_tail;
    }
    bool empty() const {
        SDL_ASSERT(!m_block_list == !m_block_tail);
        return !m_block_list;
    }
    explicit operator bool() const {
        return !empty();
    }
    using block_head_Id = std::pair<block_head *, block32>;
    block_head_Id pop_head(freelist);
    bool find_block(block32) const;
    void insert(block_head *, block32);
    bool promote(block_head *, block32);
    bool remove(block_head *, block32);
    size_t truncate(block_list_t &, size_t); // free blocks
    bool append(block_list_t &&, freelist);
    template<class fun_type>
    void for_each(fun_type &&, freelist) const;
#if SDL_DEBUG
    bool assert_list(freelist = freelist::false_, tracef = tracef::false_) const;
#endif
private:
    page_bpool_friend const m_p;
    const char * const m_name;
    block32 m_block_list = 0; // head
    block32 m_block_tail = 0;
};

template<class fun_type>
void block_list_t::for_each(fun_type && fun, freelist const f) const {
    auto p = m_block_list;
    while (p) {
        block_head * const h = m_p.first_block_head(p, f);
        SDL_DEBUG_CPP(const auto nextBlock = h->nextBlock);
        if (!fun(h, p)) {
            break;
        }
        SDL_ASSERT(nextBlock == h->nextBlock);
        p = h->nextBlock;
    }
}

}}} // sdl

#include "dataserver/bpool/block_list.inl"

#endif // __SDL_BPOOL_BLOCK_LIST_H__