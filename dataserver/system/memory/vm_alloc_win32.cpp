// vm_alloc_win32.cpp
//
#include "dataserver/system/memory/vm_alloc_win32.h"

#if defined(SDL_OS_WIN32)

namespace sdl { namespace db { namespace mmu {

vm_alloc_win32::vm_alloc_win32(uint64 const size)
    : byte_reserved(size)
    , page_reserved(size / page_size)
{
    static_assert(max_commit_page == 65536, "");
    SDL_ASSERT(size && !(size % page_size));
    SDL_ASSERT(page_reserved);
    if (page_reserved <= max_commit_page) {
        m_base_address = ::VirtualAlloc(
            NULL, // If this parameter is NULL, the system determines where to allocate the region.
            size, // The size of the region, in bytes.
            MEM_RESERVE,       // Reserves a range of the process's virtual address space without allocating any actual physical storage in memory or in the paging file on disk.
            PAGE_READWRITE);   // The memory protection for the region of pages to be allocated.
        throw_error_if<this_error>(!m_base_address, "VirtualAlloc failed");
    }
    else {
        throw_error<this_error>("bad alloc size");
    }
}

vm_alloc_win32::~vm_alloc_win32()
{
    if (m_base_address) {
        ::VirtualFree(m_base_address, 0, MEM_RELEASE);
        m_base_address = nullptr;
    }
}

bool vm_alloc_win32::check_address(uint64 const start, uint64 const size) const
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

void * vm_alloc_win32::alloc(uint64 const start, uint64 const size)
{
    if (!check_address(start, size))
		return nullptr;
    const size_t page_index = start / page_size;
    void * const lpAddress = reinterpret_cast<char*>(base_address()) + start;
    if (is_commit(page_index)) {
        return lpAddress;
    }
    // If the function succeeds, the return value is the base address of the allocated region of pages.
    void * const CommittedStorage = ::VirtualAlloc( 
        lpAddress,
        size,
        MEM_COMMIT, PAGE_READWRITE);
    if (CommittedStorage) {
        SDL_ASSERT(CommittedStorage <= lpAddress);
        SDL_ASSERT((CommittedStorage == lpAddress) && "warning");
        set_commit(page_index, true);
        return lpAddress;
    }
    throw_error<this_error>("VirtualAlloc commit failed");
    return nullptr;
}

bool vm_alloc_win32::clear(uint64 const start, uint64 const size)
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
    if (::VirtualFree(lpAddress, size, MEM_DECOMMIT)) {
        set_commit(page_index, false);
        return true;
    }
    SDL_ASSERT(0);
    return false;
}

} // mmu
} // sdl
} // db

#endif // #if defined(SDL_OS_WIN32)

