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
        throw_error_if_t<vm_win32>(!base, "VirtualAlloc MEM_RESERVE failed");
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
    const size_t endb = b + (size + block_size - 1) / block_size;
    SDL_ASSERT(b < endb);
    SDL_ASSERT(endb <= block_reserved);
    for (; b < endb; ++b) {
        if (!m_block_commit[b]) {
            char const * const lpAddress = m_base_address + b * block_size;
            // If the function succeeds, the return value is the base address of the allocated region of pages.
            void * const storage = ::VirtualAlloc((void *)lpAddress,
                alloc_block_size(b),
                MEM_COMMIT, PAGE_READWRITE);
            SDL_ASSERT(storage == lpAddress);
            if (!storage) {
                throw_error_t<vm_win32>("VirtualAlloc MEM_COMMIT failed");
                return nullptr;
            }
            m_block_commit[b] = true;
        }
    }
    return start;
}

// start and size must be aligned to blocks
bool vm_win32::release(char const * const start, const size_t size)
{
    SDL_ASSERT(assert_address(start, size));
    if ((start - m_base_address) % block_size) {
        SDL_ASSERT(0);
        return false;
    }
    char const * const end = start + size;
    if ((end - m_base_address) % block_size) {
        if (end != end_address()) {
            SDL_ASSERT(0);
            return false;
        }
    }
    size_t b = (start - m_base_address) / block_size;
    const size_t endb = b + (size + block_size - 1) / block_size;
    SDL_ASSERT(b < endb);
    SDL_ASSERT(endb <= block_reserved);
    for (; b < endb; ++b) {
        if (m_block_commit[b]) {
            char const * const lpAddress = m_base_address + b * block_size;
            if (::VirtualFree((void *)lpAddress, alloc_block_size(b), MEM_DECOMMIT)) {
                m_block_commit[b] = false;
            }
            else {
                SDL_ASSERT(0);
                throw_error_t<vm_win32>("VirtualFree MEM_DECOMMIT failed");
                return false;
            }
        }
    }
    return true;
}

#if SDL_DEBUG
namespace {
struct unit_test {
    unit_test() {
        if (1) {
            using T = vm_win32;
            enum { page_size = T::page_size };
            T test(T::block_size + T::page_size);
            for (size_t i = 0; i < test.page_reserved; ++i) {
                char const * const p = test.base_address() + i * page_size;
                if (!test.alloc(p, page_size)) {
                    SDL_ASSERT(0);
                }
            }
            SDL_ASSERT(test.release(test.base_address() + T::block_size, 
                test.byte_reserved - T::block_size));
            SDL_ASSERT(test.release());
        }
    }
};
static unit_test s_test;
}
#endif //#if SDL_DEBUG

} // sdl
} // db

#endif // SDL_OS_WIN32



