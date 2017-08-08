// lock_page.h
//
#pragma once
#ifndef __SDL_BPOOL_LOCK_PAGE_H__
#define __SDL_BPOOL_LOCK_PAGE_H__

#include "dataserver/system/page_head.h"

namespace sdl { namespace db { namespace bpool {

class page_bpool;
class lock_page_head : noncopyable {
    friend page_bpool;
    page_head const * m_p = nullptr;
    lock_page_head(page_head const * const p) noexcept : m_p(p) {
        SDL_ASSERT(m_p);
    }
public:
    lock_page_head() = default;
    lock_page_head(lock_page_head && src) noexcept
        : m_p(src.m_p) { //note: thread owner must be the same
        src.m_p = nullptr;
    }
    ~lock_page_head(); // see page_bpool.cpp
    void swap(lock_page_head & src) noexcept {
        std::swap(m_p, src.m_p); //note: thread owner must be the same
    }
    lock_page_head & operator = (lock_page_head && src) {
        lock_page_head(std::move(src)).swap(*this);
        SDL_ASSERT(!src);
        return *this;
    }
    page_head const * get() const {
        return m_p;
    }
    explicit operator bool() const {
        return m_p != nullptr;
    }
    void reset() {
        lock_page_head().swap(*this);
    }
};

}}} // sdl

#endif // __SDL_BPOOL_LOCK_PAGE_H__
