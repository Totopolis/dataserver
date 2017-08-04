// alloc.cpp
//
#include "dataserver/bpool/alloc.h"

#if 1 //defined(SDL_OS_UNIX) || SDL_DEBUG

namespace sdl { namespace db { namespace bpool {

page_bpool_alloc::page_bpool_alloc(const size_t size)
    : m_alloc(get_alloc_size(size), vm_commited::false_) // may throw
{
    SDL_ASSERT(size <= capacity());
    SDL_ASSERT(size && !(size % pool_limits::page_size));
}

bool page_bpool_alloc::release(block_list_t & free_block_list)
{
    if (!free_block_list) {
        return false; // normal case
    }
    free_block_list.for_each([this](block_head const * const p, block32 const id){
        SDL_ASSERT(get_block(id) == (char *)block_head::get_page_head(p));
        if (!m_alloc.release_block(id)) {
            SDL_ASSERT(0);
        }
    }, freelist::true_);
    free_block_list.clear();
    return true;
}

#if SDL_DEBUG
namespace {
class unit_test {
public:
    unit_test() {
        if (1) {
            enum { N = 8 };
            page_bpool_alloc test(pool_limits::block_size * N);
            SDL_ASSERT(!test.used_block());
            SDL_ASSERT(test.unused_block() >= N); // = 16
            for (size_t i = 0; i < N; ++i) {
                SDL_ASSERT(test.alloc_block());
            }
            SDL_ASSERT(0 <= test.unused_size());
            SDL_ASSERT(test.used_block() == N);
        }
    }
};
static unit_test s_test;
}
#endif // SDL_DEBUG
}}} // sdl

#endif // #if defined(SDL_OS_UNIX) || SDL_DEBUG