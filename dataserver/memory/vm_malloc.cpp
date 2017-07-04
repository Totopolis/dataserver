// vm_malloc.cpp
//
#include "dataserver/memory/vm_malloc.h"

namespace sdl { namespace db { namespace mmu {

vm_malloc::vm_malloc(uint64 const size)
    : byte_reserved(size)
    , page_reserved(size / page_size)
{
    SDL_ASSERT(page_reserved);
    if (check_alloc_size(size)) {
        m_commit.resize(page_reserved);
        m_base_address.reset(new char[byte_reserved]);
    }
    else {
        SDL_ASSERT(0);
        throw_error<this_error>("bad alloc size");
    }
}

bool vm_malloc::check_address(uint64 const start, uint64 const size) const
{
    SDL_ASSERT(start + size <= byte_reserved);
    SDL_ASSERT(size && !(size % page_size));
    SDL_ASSERT(!(start % page_size));	
    const size_t index = start / page_size;
    if ((index < page_reserved) && (start + size <= byte_reserved)) {
        return true;
    }
    throw_error<this_error>("bad page");
    return false;
}

void * vm_malloc::alloc(uint64 const start, uint64 const size)
{
    if (check_address(start, size)) {
        const size_t page_index = start / page_size;
        set_commit(page_index, true);
        return m_base_address.get() + start;
    }
    SDL_ASSERT(0);
    return nullptr;
}

void * vm_malloc::alloc_page(const size_t page_index)
{
    if (page_index <= max_page) {
        set_commit(page_index, true);
        return m_base_address.get() + (page_index * page_size);
    }
    SDL_ASSERT(0);
    return nullptr;
}

bool vm_malloc::clear(uint64 const start, uint64 const size)
{
    if (check_address(start, size)) {
        const size_t page_index = start / page_size;
        if (is_commit(page_index)) {
            set_commit(page_index, false);
            return true;
        }
    }
    SDL_ASSERT(0);
    return false;
}

} // mmu
} // sdl
} // db
