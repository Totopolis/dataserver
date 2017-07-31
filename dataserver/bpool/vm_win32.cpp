// vm_win32.cpp
//
#include "dataserver/bpool/vm_win32.h"

#if defined(SDL_OS_WIN32)
#include <windows.h>

namespace sdl { namespace db { namespace bpool {

char * vm_win32::init_vm_alloc(size_t const size, vm_commited const f) {
    if (size && !(size % block_size)) {
        void * const base = ::VirtualAlloc(
            NULL, // If this parameter is NULL, the system determines where to allocate the region.
            size, // The size of the region, in bytes.
            is_commited(f) ? (MEM_COMMIT|MEM_RESERVE) : MEM_RESERVE,
            PAGE_READWRITE);   // The memory protection for the region of pages to be allocated.
        throw_error_if_t<vm_win32>(!base, "VirtualAlloc failed");
        return reinterpret_cast<char *>(base);
    }
    throw_error_t<vm_win32>("init_vm_alloc failed");
    return nullptr;
}

vm_win32::vm_win32(size_t const size, vm_commited const f)
    : byte_reserved(size)
    , page_reserved(size / page_size)
    , block_reserved(size / block_size)
    , m_base_address(init_vm_alloc(size, f))
{
    A_STATIC_ASSERT_64_BIT;
    SDL_ASSERT(size && !(size % block_size));
    SDL_ASSERT(page_reserved <= max_page);
    SDL_ASSERT(block_reserved <= max_block);
    SDL_ASSERT(is_open());
    SDL_DEBUG_CPP(d_block_commit.resize(block_reserved, is_commited(f)));
}

vm_win32::~vm_win32()
{
    if (m_base_address) {
        ::VirtualFree(m_base_address, 0, MEM_RELEASE);
    }
}

#if SDL_DEBUG
bool vm_win32::assert_address(char const * const start, size_t const size) const {
    SDL_ASSERT(m_base_address <= start);
    SDL_ASSERT(!((start - m_base_address) % block_size));
    SDL_ASSERT(size && !(size % block_size));
    SDL_ASSERT(start + size <= end_address());
    return true;
}

size_t vm_win32::count_alloc_block() const {
    return std::count(d_block_commit.begin(), d_block_commit.end(), true);
}
#endif

char * vm_win32::alloc(char * const start, const size_t size)
{
    SDL_ASSERT(assert_address(start, size));
#if SDL_DEBUG
    {
        size_t b = (start - m_base_address) / block_size;
        const size_t endb = b + (size + block_size - 1) / block_size;
        SDL_ASSERT(b < endb);
        SDL_ASSERT(endb <= block_reserved);
        for (; b < endb; ++b) {
            SDL_ASSERT(!d_block_commit[b]);
            d_block_commit[b] = true;
        }
    }
#endif
    // the return value is the base address of the allocated region of pages.
    if (void * const storage = ::VirtualAlloc(start, size, MEM_COMMIT, PAGE_READWRITE)) {
        SDL_ASSERT(storage == start);
        return start;
    }
    throw_error_t<vm_win32>("VirtualAlloc MEM_COMMIT failed");
    return nullptr;
}

// start and size must be aligned to blocks
bool vm_win32::release(char * const start, const size_t size)
{
    SDL_ASSERT(assert_address(start, size));
#if SDL_DEBUG
    {
        size_t b = (start - m_base_address) / block_size;
        const size_t endb = b + (size + block_size - 1) / block_size;
        SDL_ASSERT(b < endb);
        SDL_ASSERT(endb <= block_reserved);
        for (; b < endb; ++b) {
            SDL_ASSERT(d_block_commit[b]);
            d_block_commit[b] = false;
        }
    }
#endif
    if (::VirtualFree(start, size, MEM_DECOMMIT)) {
        return true;
    }
    throw_error_t<vm_win32>("VirtualFree MEM_DECOMMIT failed");
    return false;
}

#if SDL_DEBUG
namespace {
class unit_test {
public:
    unit_test() {
        if (1) {
            using T = vm_win32;
            T test(T::block_size * 3, vm_commited::false_);
            for (size_t i = 0; i < test.block_reserved; ++i) {
                auto const p = test.base_address() + i * T::block_size;
                SDL_ASSERT(test.alloc(p, T::block_size));
            }
            SDL_ASSERT(test.release(test.base_address(), test.byte_reserved));
        }
    }
};
static unit_test s_test;
}
#endif // SDL_DEBUG
}}} // db
#endif // SDL_OS_WIN32



