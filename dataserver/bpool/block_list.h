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
};

class block_list_t : noncopyable {
    using block32 = block_index::block32;
public:
    enum { null = 0 };
    using value_type = block32;
    explicit block_list_t(page_bpool_friend && p, const char * const s = "")
        : m_p(p)
#if SDL_DEBUG
        , m_name(s)
#endif
    {}
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
    void clear() {
        m_block_list = null;
        m_block_tail = null;
    }
    size_t length() const; // O(N)
    using block_head_Id = std::pair<block_head *, block32>;
    block_head_Id pop_head();
    bool find_block(block32) const;
    void insert(block_head *, block32);
    bool promote(block_head *, block32);
    bool remove(block_head *, block32);
    size_t truncate(block_list_t &, size_t); // free blocks
    bool append(block_list_t &&);
    template<class fun_type>
    break_or_continue for_each(fun_type &&) const;
#if SDL_DEBUG
    bool assert_list(tracef = tracef::false_) const;
    void trace() const;
#endif
    void for_each_insert(interval_block32 &) const;
private:
    page_bpool_friend const m_p;
    SDL_DEBUG_HPP(const char * const m_name;)
    block32 m_block_list = 0; // head
    block32 m_block_tail = 0;
};

template<class fun_type> break_or_continue
block_list_t::for_each(fun_type && fun) const {
    block32 p = m_block_list;
    while (p) {
        block_head * const h = m_p.first_block_head(p);
        block32 const nextBlock = h->nextBlock;
        if (is_break(fun(h, p))) {
            return bc::break_;
        }
        p = nextBlock; // can't use h (see page_bpool_alloc_unix::release(), block can be deleted)
    }
    return bc::continue_;
}

inline void block_list_t::for_each_insert(interval_block32 & dest) const {
    for_each([&dest](block_head *, block32 p){
        dest.insert(p);
        return bc::continue_;
    });
}

}}} // sdl

#include "dataserver/bpool/block_list.inl"

#endif // __SDL_BPOOL_BLOCK_LIST_H__
