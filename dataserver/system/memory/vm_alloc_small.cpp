// vm_alloc_small.cpp
//
#include "dataserver/system/memory/vm_alloc_small.h"

#if defined(SDL_OS_WIN32) || SDL_DEBUG_SMALL_MEMORY
namespace sdl { namespace db { namespace mmu {

vm_alloc_small::vm_alloc_small(uint64 const size)
    : byte_reserved(size)
    , page_reserved(size / page_size)
{
    SDL_ASSERT(size && !(size % page_size));
    SDL_ASSERT(page_reserved);
    slots.resize(page_reserved);
}

bool vm_alloc_small::check_address(uint64 const start, uint64 const size) const
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

void * vm_alloc_small::alloc(uint64 const start, uint64 const size)
{
    if (check_address(start, size)) {
        const size_t first = start / page_size;
        const size_t last  = first + size / page_size;
        for (size_t i = first; i < last; ++i) {
            auto & p = slots[i];
            if (!p) {
                p.reset(new char[page_size]);
            }
        }
        return slots[first].get();
    }
    return nullptr;
}

bool vm_alloc_small::clear(uint64 const start, uint64 const size)
{
    if (check_address(start, size)) {
        auto & p = slots[start / page_size];
        if (p) {
            p.reset();
            return true;
        }
    }
    SDL_ASSERT(0);
    return false;
}

} // mmu
} // sdl
} // db

#endif // #if defined(SDL_OS_WIN32) || SDL_DEBUG_SMALL_MEMORY