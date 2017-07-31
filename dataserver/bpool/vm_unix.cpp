// vm_unix.cpp
//
#include "dataserver/bpool/vm_unix.h"
#include "dataserver/filesys/mmap64_unix.h"

#if defined(SDL_OS_UNIX)
    #if !defined(MAP_ANONYMOUS)
        #define MAP_ANONYMOUS MAP_ANON
    #endif
#endif

#if defined(SDL_OS_UNIX) || SDL_DEBUG
namespace sdl { namespace db { namespace bpool {

char * vm_unix::init_vm_alloc(size_t const size, vm_commited const f) {
#if defined(SDL_OS_UNIX)
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
#endif // SDL_OS_UNIX
    throw_error_t<vm_unix>("init_vm_alloc failed");
    return nullptr;
}

vm_unix::vm_unix(size_t const size, vm_commited const f)
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

vm_unix::~vm_unix()
{
    if (m_base_address) {
#if defined(SDL_OS_UNIX)
        if (::munmap(m_base_address, byte_reserved)) {
            SDL_ASSERT(!"munmap");
        }
#endif
    }
}

#if SDL_DEBUG
bool vm_unix::assert_address(char const * const start, size_t const size) const {
    SDL_ASSERT(m_base_address <= start);
    SDL_ASSERT(!((start - m_base_address) % block_size));
    SDL_ASSERT(size && !(size % block_size));
    SDL_ASSERT(start + size <= end_address());
    return true;
}

size_t vm_unix::count_alloc_block() const {
    return std::count(d_block_commit.begin(), d_block_commit.end(), true);
}
#endif

char * vm_unix::alloc(char * const start, const size_t size)
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
    return start;
}

// start and size must be aligned to blocks
bool vm_unix::release(char * const start, const size_t size)
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
    return true;
}

#if SDL_DEBUG
namespace {
class unit_test {
public:
    unit_test() {
        if (0) {
            using T = vm_unix;
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