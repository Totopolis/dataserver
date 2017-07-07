// vm_win32.cpp
//
#include "dataserver/system/vm/vm_win32.h"

#if defined(SDL_OS_WIN32)

#include <windows.h>

namespace sdl { namespace db {

char const * vm_win32::init_vm_alloc(size_t const size) {
    if (size && !(size % page_size)) {
        void * const base = ::VirtualAlloc(
            NULL, // If this parameter is NULL, the system determines where to allocate the region.
            size, // The size of the region, in bytes.
            MEM_RESERVE,       // Reserves a range of the process's virtual address space without allocating any actual physical storage in memory or in the paging file on disk.
            PAGE_READWRITE);   // The memory protection for the region of pages to be allocated.
        throw_error_if_t<vm_win32>(!base, "VirtualAlloc failed");
        return reinterpret_cast<char const *>(base);
    }
    SDL_ASSERT(0);
    return nullptr;
}

vm_win32::vm_win32(size_t const size)
    : byte_reserved(size)
    , page_reserved(size / page_size)
    , slot_reserved((size + slot_size - 1) / slot_size)
    , block_reserved((size + block_size - 1) / block_size)
    , m_base_address(init_vm_alloc(size))
{
    A_STATIC_ASSERT_64_BIT;
    SDL_ASSERT(size && !(size % page_size));
    SDL_ASSERT(page_reserved * page_size == size);
    SDL_ASSERT(page_reserved <= max_page);
    SDL_ASSERT(slot_reserved <= max_slot);
    SDL_ASSERT(block_reserved <= max_block);
    SDL_ASSERT(is_open());
    m_block_commit.resize(block_reserved);
}

vm_win32::~vm_win32()
{
    if (m_base_address) {
        ::VirtualFree((void *)(m_base_address), 0, MEM_RELEASE);
    }
}

char const * vm_win32::alloc(char const * const start, const size_t size)
{
    SDL_ASSERT(assert_address(start, size));
    size_t b = (start - m_base_address) / block_size;
    const size_t end = b + (size + block_size - 1) / block_size;
    SDL_ASSERT(b < end);
    for (; b < end; ++b) {
        if (!m_block_commit[b]) {
        }
    }
    return start;
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
            using T = vm_win32;
            enum { N = 32 };
            enum { page_size = T::page_size };
            T test(T::block_size + T::page_size);
            for (size_t i = 0; i < N; ++i) {
                const auto p = test.base_address() + i * page_size;
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



