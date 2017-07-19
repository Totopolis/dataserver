// block_list.h
//
#pragma once
#ifndef __SDL_BPOOL_BLOCK_LIST_H__
#define __SDL_BPOOL_BLOCK_LIST_H__

#include "dataserver/bpool/block_head.h"

namespace sdl { namespace db { namespace bpool {

class page_bpool;
class block_list_t : noncopyable {
    using block32 = block_index::block32;
    enum { null = 0 };
public:
    block_list_t(page_bpool * const p, const char * const s)
        : m_p(p), m_name(s)
    {
        SDL_ASSERT(m_p);
        SDL_ASSERT(m_name);
    }
    block32 head() const {
        return m_block_list;
    }
    bool empty() const {
        return !m_block_list;
    }
    explicit operator bool() const {
        return !empty();
    }
    void insert(block_head *, block32);
    bool promote(block_head *, block32);
    void remove(block_head *, block32);
    bool find_block(block32) const;
//private:
#if SDL_DEBUG
    enum { trace_enable = 0 };
    bool assert_list() const;
#endif
private:
    page_bpool * const m_p;
    const char * const m_name;
    block32 m_block_list = 0; // head
};

}}} // sdl

#endif // __SDL_BPOOL_BLOCK_LIST_H__
