// alloc.h
//
#pragma once
#ifndef __SDL_BPOOL_ALLOC_H__
#define __SDL_BPOOL_ALLOC_H__

#if defined(SDL_OS_WIN32)
#include "dataserver/bpool/vm_win32.h"
#else
#include "dataserver/bpool/vm_unix.h"
#endif
#include "dataserver/bpool/block_head.h"

namespace sdl { namespace db { namespace bpool {

#if defined(SDL_OS_WIN32)
using vm_alloc = vm_win32;
#else
using vm_alloc = vm_unix;
#endif

class page_bpool_alloc final : noncopyable { // to be improved
public:
    explicit page_bpool_alloc(const size_t size)
        : m_alloc(size, vm_commited::false_) {
        m_alloc_brk = base();
        throw_error_if_t<page_bpool_alloc>(!m_alloc_brk, "bad alloc");
    }
    char * base() const {
        return m_alloc.base_address();
    }
    size_t capacity() const {
        return m_alloc.byte_reserved;
    }
    size_t used_size() const {
        SDL_ASSERT(assert_brk());
        return m_alloc_brk - m_alloc.base_address();
    }
    size_t unused_size() const {
        return m_alloc.byte_reserved - used_size();
    }
    bool can_alloc(const size_t size) const {
        SDL_ASSERT(size && !(size % pool_limits::block_size));
        return size <= unused_size();
    }
    char * alloc(const size_t size) {
        SDL_ASSERT(size && !(size % pool_limits::block_size));
        if (size <= unused_size()) {
            if (auto result = m_alloc.alloc(m_alloc_brk, size)) {
                SDL_ASSERT(result == m_alloc_brk);
                m_alloc_brk += size;
                SDL_ASSERT(assert_brk());
                return result;
            }
        }
        SDL_ASSERT(0);
        throw_error_t<page_bpool_alloc>("bad alloc");
        return nullptr;
    }
    size_t block_id(char const * const p) const {
        SDL_ASSERT(p >= m_alloc.base_address());
        SDL_ASSERT(p < m_alloc_brk);
        const size_t size = p - base();
        SDL_ASSERT(!(size % pool_limits::block_size));
        return size / pool_limits::block_size;
    }
    char * get_block(size_t const id) const {
        char * const p = base() + id * pool_limits::block_size;
        SDL_ASSERT(p >= m_alloc.base_address());
        SDL_ASSERT(p < m_alloc_brk);
        return p;
    }
private:
    bool assert_brk() const {
        SDL_ASSERT(m_alloc_brk >= m_alloc.base_address());
        SDL_ASSERT(m_alloc_brk <= m_alloc.end_address());
        return true;
    }
private:
    vm_alloc m_alloc;
    char * m_alloc_brk = nullptr; // end of allocated space
};

}}} // sdl

#endif // __SDL_BPOOL_ALLOC_H__
