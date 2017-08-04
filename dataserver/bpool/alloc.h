// alloc.h
//
#pragma once
#ifndef __SDL_BPOOL_ALLOC_H__
#define __SDL_BPOOL_ALLOC_H__

#if defined(SDL_OS_WIN32)
#include "dataserver/bpool/vm_win32.h"
#endif
#if defined(SDL_OS_UNIX) || SDL_DEBUG
#include "dataserver/bpool/vm_unix.h"
#endif
#include "dataserver/bpool/block_list.h"

namespace sdl { namespace db { namespace bpool {

#if defined(SDL_OS_WIN32)
    #if 0 //SDL_DEBUG
        using vm_alloc = vm_unix_new;
    #else
        using vm_alloc = vm_win32;
    #endif
#else
    using vm_alloc = vm_unix_old;
#endif

class page_bpool_alloc final : noncopyable { // to be improved
    using block32 = block_index::block32;
public:
    enum { block_size = pool_limits::block_size };  
    explicit page_bpool_alloc(size_t);
    bool is_open() const {
        return m_alloc.is_open();
    }
    size_t capacity() const {
        return m_alloc.byte_reserved;
    }
    size_t used_size() const {
        SDL_ASSERT(assert_brk());
        const size_t brk = m_alloc_brk - m_alloc.base_address();
        SDL_ASSERT(brk >= decommit_size());
        return brk - decommit_size();
    }
    size_t used_block() const {
        return used_size() / block_size;
    }
    size_t unused_size() const {
        return m_alloc.byte_reserved - used_size();
    }
    size_t unused_block() const {
        return unused_size() / block_size;
    }
    bool can_alloc(const size_t size) const {
        SDL_ASSERT(size && !(size % block_size));
        return size <= unused_size();
    }
    char * alloc_block();
    block32 get_block_id(char const *) const; // block must be allocated
    char * get_block(block32) const; // block must be allocated
    bool release(block_list_t &); // release/decommit memory
private:
    size_t decommit_block() const {
        return m_decommit.size();
    }
    size_t decommit_size() const {
        return decommit_block() * block_size;
    }
    static constexpr size_t get_alloc_size(const size_t size) {
        return round_up_div(size, (size_t)block_size) * block_size;
    }
#if SDL_DEBUG
    bool assert_brk() const;
#endif
private:
    using interval_block32 = interval_set<block32>;
    vm_alloc m_alloc;
    char * m_alloc_brk = nullptr; // end of allocated space (brk = break)
    interval_block32 m_decommit; // added to unused_size
};

}}} // sdl

#include "dataserver/bpool/alloc.inl"

#endif // __SDL_BPOOL_ALLOC_H__