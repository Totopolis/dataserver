// vm_alloc_win32.cpp
//
#include "dataserver/system/memory/vm_alloc_win32.h"

#if defined(SDL_OS_WIN32)

namespace sdl { namespace db { namespace mmu {

vm_alloc_win32::vm_alloc_win32(uint64 const size)
    : byte_reserved(size)
    , max_page(size / page_size)
{
    SDL_ASSERT(size && !(size % page_size));
    SDL_ASSERT(max_page);
    m_base_address = ::VirtualAlloc(
        NULL, // If this parameter is NULL, the system determines where to allocate the region.
        size, // The size of the region, in bytes.
        MEM_RESERVE,       // Reserves a range of the process's virtual address space without allocating any actual physical storage in memory or in the paging file on disk.
        PAGE_READWRITE);   // The memory protection for the region of pages to be allocated.
    throw_error_if<this_error>(!m_base_address, "VirtualAlloc failed");
}

vm_alloc_win32::~vm_alloc_win32()
{
    if (m_base_address) {
        ::VirtualFree(m_base_address, 0, MEM_RELEASE);
        m_base_address = nullptr;
    }
}

void * vm_alloc_win32::alloc(uint64 const start, uint64 const size)
{
    SDL_ASSERT(start + size <= byte_reserved);
    SDL_ASSERT(size && !(size % page_size));
    SDL_ASSERT(!(start % page_size));
    const size_t index = start / page_size;
    if ((max_page <= index) || (byte_reserved < start + size)) {
        throw_error<this_error>("bad alloc");
        return nullptr;
    }
    void * const p = reinterpret_cast<char*>(base_address()) + start;
    SDL_WARNING(0);
    return nullptr;
}

void vm_alloc_win32::clear(uint64 const start, uint64 const size)
{
    SDL_ASSERT(start + size <= byte_reserved);
    SDL_ASSERT(size && !(size % page_size));
    SDL_ASSERT(!(start % page_size));
    const size_t index = start / page_size;
    if ((max_page <= index) || (byte_reserved < start + size)) {
        throw_error<this_error>("bad free");
        return;
    }
    void * const p = reinterpret_cast<char*>(base_address()) + start;
    SDL_WARNING(0);
}

} // mmu
} // sdl
} // db

#endif // #if defined(SDL_OS_WIN32)

