// page_pool_type.cpp
//
#include "dataserver/system/page_pool_type.h"

namespace sdl { namespace db { namespace bpool {

#if SDL_DEBUG
namespace {
    class unit_test {
    public:
        unit_test() {
            using T = pool_limits;
            A_STATIC_ASSERT_IS_POD(block_head);
            static_assert(sizeof(block_head) == sizeof(uint32), "");
            static_assert(T::max_bindex * T::block_size == terabyte<1>::value, "");
            {
                block_head test{};
                test.d.index = T::last_bindex;
                SDL_ASSERT(T::last_bindex == test.value);
                SDL_ASSERT(T::last_bindex == test.d.index);
                SDL_ASSERT(0 == test.d.pages);
                for (size_t i = 0; i < T::block_page_num; ++i) {
                    test.set_page(i);
                    SDL_ASSERT(test.page(i));
                    SDL_ASSERT(test.d.pages == (1 << i));
                    test.clear_page(i);
                }
                SDL_ASSERT(0 == test.d.pages);
            }
        }
    };
    static unit_test s_test;
}
#endif
}}} // sdl
