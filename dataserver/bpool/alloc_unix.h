// alloc_unix.h
//
#pragma once
#ifndef __SDL_BPOOL_ALLOC_UNIX_H__
#define __SDL_BPOOL_ALLOC_UNIX_H__

#include "dataserver/bpool/vm_unix.h"
#include "dataserver/bpool/block_list.h"

namespace sdl { namespace db { namespace bpool {

class page_bpool_alloc_unix final : noncopyable {
    using block32 = block_index::block32;
public:
    enum { block_size = pool_limits::block_size };  
    static constexpr size_t get_alloc_size(const size_t size) {
        return round_up_div(size, (size_t)block_size) * block_size;
    }
    explicit page_bpool_alloc_unix(size_t);
    bool is_open() const {
        return true;
    }
    size_t capacity() const {
        return m_alloc.byte_reserved;
    }
    size_t used_size() const {
        return m_alloc.used_size();
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
    char * alloc_block() {
        return m_alloc.alloc_block();
    }
    void release_list(block_list_t &); // release/decommit memory
    template <class fun_type>
    void defragment(fun_type && fun)  {
        m_alloc.defragment(fun);
    }
    block32 get_block_id(char const * block_adr) const { // block must be allocated
        return m_alloc.get_block_id(block_adr);
    }
    char * get_block(block32 const id) const { // block must be allocated
        return m_alloc.get_block(id);
    }
    size_t alloc_block_count() const {
        return m_alloc.alloc_block_count();
    }
#if SDL_DEBUG || defined(SDL_TRACE_RELEASE)
    void trace() const;
#endif
private:
    vm_unix m_alloc;
};

}}} // sdl

#endif // __SDL_BPOOL_ALLOC_UNIX_H__