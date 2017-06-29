// vm_alloc_unix.cpp
//
#if defined(SDL_OS_UNIX)

#include "dataserver/memory/vm_alloc_unix.h"
#include "dataserver/memory/mmap64_unix.hpp"

#if !defined(MAP_ANONYMOUS)
#define MAP_ANONYMOUS MAP_ANON
#endif

namespace sdl { namespace db { namespace mmu {

vm_alloc_unix::vm_alloc_unix(uint64 const size)
    : byte_reserved(size)
    , page_reserved(size / page_size)
{
    static_assert(max_commit_page == 65536, "");
    SDL_ASSERT(size && !(size % page_size));
    SDL_ASSERT(page_reserved);
    if (page_reserved <= max_commit_page) {
        m_base_address = mmap64_t::call(
            nullptr, static_cast<size_t>(size), 
            PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        throw_error_if<this_error>(!m_base_address, "vm_alloc_unix failed");
    }
    else {
        throw_error<this_error>("bad alloc size");
    }
}

vm_alloc_unix::~vm_alloc_unix()
{
    if (m_base_address) {
        if (::munmap(m_base_address, byte_reserved)) {
            SDL_ASSERT(!"munmap");
        }
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
    set_commit(page_index, true);
    return lpAddress;
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
    set_commit(page_index, false);
    return true;
}

} // mmu
} // sdl
} // db

#endif // #if defined(SDL_OS_UNIX)


#if 0
/*
http://man7.org/linux/man-pages/man2/mmap.2.html
#include <sys/mman.h>
void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int munmap(void *addr, size_t length);
*/

#if !defined(MAP_ANONYMOUS)
#define MAP_ANONYMOUS MAP_ANON
#endif

static void
munmap_checked(void *addr, size_t size)
{
	if (munmap(addr, size)) {
		char buf[64];
		intptr_t ignore_it = (intptr_t)strerror_r(errno, buf,
							  sizeof(buf));
		(void)ignore_it;
		fprintf(stderr, "Error in munmap(%p, %zu): %s\n",
			addr, size, buf);
		assert(false);
	}
}

static void *
mmap_checked(size_t size, size_t align, int flags)
{
	/* The alignment must be a power of two. */
	assert((align & (align - 1)) == 0);
	/* The size must be a multiple of alignment */
	assert((size & (align - 1)) == 0);
	/*
	 * All mappings except the first are likely to
	 * be aligned already.  Be optimistic by trying
	 * to map exactly the requested amount.
	 */
	void *map = mmap(NULL, size, PROT_READ | PROT_WRITE,
			 flags | MAP_ANONYMOUS, -1, 0);
	if (map == MAP_FAILED)
		return NULL;
	if (((intptr_t) map & (align - 1)) == 0)
		return map;
	munmap_checked(map, size);

	/*
	 * mmap enough amount to be able to align
	 * the mapped address.  This can lead to virtual memory
	 * fragmentation depending on the kernels allocation
	 * strategy.
	 */
	map = mmap(NULL, size + align, PROT_READ | PROT_WRITE,
		   flags | MAP_ANONYMOUS, -1, 0);
	if (map == MAP_FAILED)
		return NULL;

	/* Align the mapped address around slab size. */
	size_t offset = (intptr_t) map & (align - 1);

	if (offset != 0) {
		/* Unmap unaligned prefix and postfix. */
		munmap_checked(map, align - offset);
		map += align - offset;
		munmap_checked(map + size, offset);
	} else {
		/* The address is returned aligned. */
		munmap_checked(map + size, align);
	}
	return map;
}
#endif