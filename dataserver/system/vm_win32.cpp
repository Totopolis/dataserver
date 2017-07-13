// vm_win32.cpp
//
#include "dataserver/system/vm_win32.h"

#if defined(SDL_OS_WIN32)
#include <windows.h>

namespace sdl { namespace db { namespace bpool {

char * vm_win32::init_vm_alloc(size_t const size, bool const commited) {
    SDL_TRACE(__FUNCTION__, " ", (commited ? "MEM_COMMIT|" : ""), "MEM_RESERVE");
    if (size && !(size % page_size)) {
        void * const base = ::VirtualAlloc(
            NULL, // If this parameter is NULL, the system determines where to allocate the region.
            size, // The size of the region, in bytes.
            commited ? MEM_COMMIT|MEM_RESERVE : MEM_RESERVE,
            PAGE_READWRITE);   // The memory protection for the region of pages to be allocated.
        throw_error_if_t<vm_win32>(!base, "VirtualAlloc failed");
        return reinterpret_cast<char *>(base);
    }
    SDL_ASSERT(0);
    return nullptr;
}

vm_win32::vm_win32(size_t const size, bool const commited)
    : byte_reserved(size)
    , page_reserved(size / page_size)
    , slot_reserved((size + slot_size - 1) / slot_size)
    , block_reserved((size + block_size - 1) / block_size)
    , m_base_address(init_vm_alloc(size, commited))
{
    A_STATIC_ASSERT_64_BIT;
    SDL_ASSERT(size && !(size % page_size));
    SDL_ASSERT(page_reserved * page_size == size);
    SDL_ASSERT(page_reserved <= max_page);
    SDL_ASSERT(slot_reserved <= max_slot);
    SDL_ASSERT(block_reserved <= max_block);
    SDL_ASSERT(is_open());
    m_block_commit.resize(block_reserved, commited);
}

vm_win32::~vm_win32()
{
    if (m_base_address) {
        ::VirtualFree(m_base_address, 0, MEM_RELEASE);
    }
}

bool vm_win32::is_alloc(char * const start, const size_t size) const
{
    SDL_ASSERT(assert_address(start, size));
    size_t b = (start - m_base_address) / block_size;
    const size_t endb = b + (size + block_size - 1) / block_size;
    SDL_ASSERT(b < endb);
    SDL_ASSERT(endb <= block_reserved);
    for (; b < endb; ++b) {
        if (!m_block_commit[b]) {
            return false;
        }
    }
    return true;
}

char * vm_win32::alloc(char * const start, const size_t size)
{
    SDL_ASSERT(assert_address(start, size));
    size_t b = (start - m_base_address) / block_size;
    const size_t endb = b + (size + block_size - 1) / block_size;
    SDL_ASSERT(b < endb);
    SDL_ASSERT(endb <= block_reserved);
    for (; b < endb; ++b) {
        if (!m_block_commit[b]) {
            char * const lpAddress = m_base_address + b * block_size;
            void * const storage = ::VirtualAlloc(lpAddress,
                alloc_block_size(b),
                MEM_COMMIT, PAGE_READWRITE); // the return value is the base address of the allocated region of pages.
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
bool vm_win32::release(char * const start, const size_t size)
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
            char * const lpAddress = m_base_address + b * block_size;
            if (::VirtualFree(lpAddress, alloc_block_size(b), MEM_DECOMMIT)) {
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
class unit_test {
public:
    unit_test() {
        if (0) {
            using T = vm_win32;
            T test(T::block_size + T::page_size, false);
            for (size_t i = 0; i < test.page_reserved; ++i) {
                auto const p = test.base_address() + i * T::page_size;
                if (!test.alloc(p, T::page_size)) {
                    SDL_ASSERT(0);
                }
            }
            SDL_ASSERT(test.release(test.base_address() + T::block_size, 
                test.byte_reserved - T::block_size));
            SDL_ASSERT(test.release_all());
        }
        if (0) {
            using T = vm_test;
            T test(T::block_size + T::page_size, false);
            for (size_t i = 0; i < test.page_reserved; ++i) {
                auto const p = test.base_address() + i * T::page_size;
                if (!test.alloc(p, T::page_size)) {
                    SDL_ASSERT(0);
                }
            }
            SDL_ASSERT(test.release(test.base_address() + T::block_size, 
                test.byte_reserved - T::block_size));
            SDL_ASSERT(test.release_all());
        }
    }
};
static unit_test s_test;
}
#endif // SDL_DEBUG
}}} // db
#endif // SDL_OS_WIN32



