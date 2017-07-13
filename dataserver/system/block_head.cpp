// block_head.cpp
//
#include "dataserver/system/block_head.h"

namespace sdl { namespace db { namespace bpool {

#if SDL_DEBUG
namespace {
    class unit_test {
    public:
        unit_test() {
            A_STATIC_ASSERT_IS_POD(block_head);
            using T = pool_limits;
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
                for (uint32 i = 0; i < T::max_bindex; ++i) {
                    test.set_index(i);
                    SDL_ASSERT(test.index() == i);
                    SDL_ASSERT(0 == test.d.pages);
                }
                SDL_ASSERT(T::last_bindex == test.value);
            }
            A_STATIC_ASSERT_IS_POD(block_page_head);
            static_assert(sizeof(block_page_head) == page_head::reserved_size, "");
        }
    };
    static unit_test s_test;
}
#endif
}}} // sdl
