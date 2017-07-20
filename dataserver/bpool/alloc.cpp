// alloc.cpp
//
#include "dataserver/bpool/alloc.h"

namespace sdl { namespace db { namespace bpool {

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