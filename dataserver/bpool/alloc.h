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
    using block32 = block_index::block32;
    enum { can_throw = false };
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
    char * alloc(size_t);
    block32 block_id(char const * block_adr) const;
    char * get_block(block32) const;
private:
#if SDL_DEBUG
    bool assert_brk() const {
        SDL_ASSERT(m_alloc_brk >= m_alloc.base_address());
        SDL_ASSERT(m_alloc_brk <= m_alloc.end_address());
        return true;
    }
#endif
private:
    vm_alloc m_alloc;
    char * m_alloc_brk = nullptr; // end of allocated space
};

}}} // sdl

#include "dataserver/bpool/alloc.inl"

#endif // __SDL_BPOOL_ALLOC_H__