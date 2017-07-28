// alloc.cpp
//
#include "dataserver/bpool/alloc.h"

namespace sdl { namespace db { namespace bpool {

bool page_bpool_alloc::decommit(block_list_t & free_block_list)
{
    SDL_TRACE(__FUNCTION__, " = ", free_block_list.length());
    if (!free_block_list) {
        SDL_ASSERT(0);
        return false;
    }
    if (0) {
        free_block_list.for_each([this](block_head const * const p, block32 const id){
            page_head const * const page = block_head::get_page_head(p);
            SDL_ASSERT((char *)page == get_block(id));
            return true;
        }, freelist::true_);
        free_block_list.clear();
    }
    return false;
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
            SDL_ASSERT(test.unused_block() == N);
            for (size_t i = 0; i < N; ++i) {
                SDL_ASSERT(test.alloc(pool_limits::block_size));
            }
            SDL_ASSERT(0 == test.unused_size());
            SDL_ASSERT(!test.can_alloc(pool_limits::block_size));
            SDL_ASSERT(test.used_block() == N);
            SDL_ASSERT(!test.unused_block());
        }
    }
};
static unit_test s_test;
}
#endif // SDL_DEBUG
}}} // sdl