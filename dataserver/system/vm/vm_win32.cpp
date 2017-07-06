// vm_win32.cpp
//
#include "dataserver/system/vm/vm_win32.h"

#if defined(SDL_OS_WIN32)

namespace sdl { namespace db {

vm_win32::vm_win32(size_t const size)
    : byte_reserved(size)
    , page_reserved(size / page_size)
{
    A_STATIC_ASSERT_64_BIT;
    SDL_ASSERT(size && !(size % page_size));
    SDL_ASSERT(page_reserved * page_size == size);
    SDL_ASSERT(page_reserved < max_page);
    if (size && !(size % page_size)) {
        m_base_address = (char *)::VirtualAlloc(
            NULL, // If this parameter is NULL, the system determines where to allocate the region.
            size, // The size of the region, in bytes.
            MEM_RESERVE,       // Reserves a range of the process's virtual address space without allocating any actual physical storage in memory or in the paging file on disk.
            PAGE_READWRITE);   // The memory protection for the region of pages to be allocated.
    }
    throw_error_if_not_t<vm_win32>(is_open(), "VirtualAlloc failed");
}

vm_win32::~vm_win32()
{
    if (m_base_address) {
        ::VirtualFree(m_base_address, 0, MEM_RELEASE);
    }
}

void * vm_win32::alloc(char const * const start, const size_t size)
{
    SDL_ASSERT(assert_address(start, size));
    return nullptr;
}

void vm_win32::release(char const * const start, const size_t size)
{
    SDL_ASSERT(assert_address(start, size));
}

#if 0
void * vm_win32::alloc(uint64 const start, uint64 const size)
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

bool vm_win32::release(uint64 const start, uint64 const size)
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
#endif

#if SDL_DEBUG
namespace {
struct unit_test {
    unit_test() {
        if (1) {
            enum { N = 16 };
            enum { page_size = page_head::page_size };
            enum { reserve = page_size * N };
            vm_win32 test(reserve);
            for (size_t i = 0; i < N; ++i) {
                const auto p = test.get() + i * page_size;
                if (test.alloc(p, page_size)) {
                    test.release(p, page_size);
                }
            }
        }
    }
};
static unit_test s_test;
}
#endif //#if SDL_DEBUG

} // sdl
} // db

#endif // SDL_OS_WIN32



