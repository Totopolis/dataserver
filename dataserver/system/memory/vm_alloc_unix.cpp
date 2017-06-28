// vm_alloc_unix.cpp
//
#include "dataserver/system/memory/vm_alloc_unix.h"

#if defined(SDL_OS_UNIX)

namespace sdl { namespace db { namespace mmu {

vm_alloc_unix::vm_alloc_unix(uint64 const size)
    : byte_reserved(size)
    , page_reserved(size / page_size)
{
    static_assert(max_commit_page == 65536, "");
    SDL_ASSERT(size && !(size % page_size));
    SDL_ASSERT(page_reserved);
    if (page_reserved <= max_commit_page) {
        throw_error_if<this_error>(!m_base_address, "vm_alloc_unix failed");
    }
    else {
        throw_error<this_error>("bad alloc size");
    }
}

vm_alloc_unix::~vm_alloc_unix()
{
    if (m_base_address) {
        m_base_address = nullptr;
    }
}

bool vm_alloc_unix::check_address(uint64 const start, uint64 const size) const
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

void * vm_alloc_unix::alloc(uint64 const start, uint64 const size)
{
    if (!check_address(start, size))
		return nullptr;
    const size_t page_index = start / page_size;
    void * const lpAddress = reinterpret_cast<char*>(base_address()) + start;
    if (is_commit(page_index)) {
        return lpAddress;
    }
    throw_error<this_error>("vm_alloc_unix::alloc failed");
    return nullptr;
}

bool vm_alloc_unix::clear(uint64 const start, uint64 const size)
{
    if (!check_address(start, size)) {
        return false;
    }
    const size_t page_index = start / page_size;
    if (!is_commit(page_index)) {
        SDL_ASSERT(0);
        return false;
    }
    void * const lpAddress = reinterpret_cast<char*>(base_address()) + start;
    if (false) {
        set_commit(page_index, false);
        return true;
    }
    SDL_ASSERT(0);
    return false;
}

} // mmu
} // sdl
} // db

#endif // #if defined(SDL_OS_UNIX)

