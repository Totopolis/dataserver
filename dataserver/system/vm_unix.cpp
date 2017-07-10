// vm_unix.cpp
//
#include "dataserver/system/vm_unix.h"
#include "dataserver/filesys/mmap64_unix.h"

#if defined(SDL_OS_UNIX)

#if !defined(MAP_ANONYMOUS)
#define MAP_ANONYMOUS MAP_ANON
#endif

namespace sdl { namespace db {

char * vm_unix::init_vm_alloc(size_t const size, bool const commited) {
    SDL_TRACE(__FUNCTION__, " ", (commited ? "MEM_COMMIT|" : ""), "MEM_RESERVE");
    if (size && !(size % page_size)) {
        void * const base = mmap64_t::call(nullptr, size, 
            PROT_READ | PROT_WRITE, // the desired memory protection of the mapping
            MAP_PRIVATE | MAP_ANONYMOUS // private copy-on-write mapping. The mapping is not backed by any file
            //| MAP_UNINITIALIZED // Don't clear anonymous pages
            ,-1 // file descriptor
            , 0 // offset must be a multiple of the page size as returned by sysconf(_SC_PAGE_SIZE)
        );
        throw_error_if_t<vm_unix>(!base, "mmap64_t failed");
        return reinterpret_cast<char *>(base);
    }
    SDL_ASSERT(0);
    return nullptr;
}

vm_unix::vm_unix(size_t const size, bool const commited)
    : byte_reserved(size)
    , page_reserved(size / page_size)
    , slot_reserved((size + slot_size - 1) / slot_size)
    , block_reserved((size + block_size - 1) / block_size)
    , m_base_address(init_vm_alloc(size, true)) // commited = true
{
    A_STATIC_ASSERT_64_BIT;
    SDL_ASSERT(commited);
    SDL_ASSERT(size && !(size % page_size));
    SDL_ASSERT(page_reserved * page_size == size);
    SDL_ASSERT(page_reserved <= max_page);
    SDL_ASSERT(slot_reserved <= max_slot);
    SDL_ASSERT(block_reserved <= max_block);
    SDL_ASSERT(is_open());
    m_block_commit.resize(block_reserved, true); // commited = true
}

vm_unix::~vm_unix()
{
    if (m_base_address) {
        if (::munmap(m_base_address, byte_reserved)) {
            SDL_ASSERT(!"munmap");
        }
    }
}

bool vm_unix::is_alloc(char * const start, const size_t size) const
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

char * vm_unix::alloc(char * const start, const size_t size)
{
    SDL_ASSERT(assert_address(start, size));
    size_t b = (start - m_base_address) / block_size;
    const size_t endb = b + (size + block_size - 1) / block_size;
    SDL_ASSERT(b < endb);
    SDL_ASSERT(endb <= block_reserved);
    for (; b < endb; ++b) {
        //if (!m_block_commit[b]) {
            m_block_commit[b] = true;
        //}
    }
    return start;
}

// start and size must be aligned to blocks
bool vm_unix::release(char * const start, const size_t size)
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
        //if (m_block_commit[b]) {
            m_block_commit[b] = false;
        //}
    }
    return true;
}

#if SDL_DEBUG
namespace {
class unit_test {
public:
    unit_test() {
        if (0) {
            using T = vm_unix;
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
} // sdl
} // db
#endif // SDL_OS_UNIX

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