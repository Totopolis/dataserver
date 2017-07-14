// block_head.cpp
//
#include "dataserver/bpool/block_head.h"

namespace sdl { namespace db { namespace bpool {

#if SDL_DEBUG
namespace {
    class unit_test {
    public:
        unit_test() {
            A_STATIC_ASSERT_IS_POD(block_index);
            using T = pool_limits;
            static_assert(sizeof(block_index) == sizeof(uint32), "");
            static_assert(T::max_block * T::block_size == terabyte<1>::value, "");
            static constexpr size_t last_block = T::max_block - 1;
            {
                block_index test{};
                test.d.index = last_block;
                SDL_ASSERT(last_block == test.value);
                SDL_ASSERT(last_block == test.d.index);
                SDL_ASSERT(0 == test.d.pages);
                for (size_t i = 0; i < T::block_page_num; ++i) {
                    test.set_page(i);
                    SDL_ASSERT(test.page(i));
                    SDL_ASSERT(test.d.pages == (1 << i));
                    test.clear_page(i);
                }
                for (uint32 i = 0; i < T::max_block; ++i) {
                    test.set_index(i);
                    SDL_ASSERT(test.index() == i);
                    SDL_ASSERT(0 == test.d.pages);
                }
                SDL_ASSERT(last_block == test.value);
            }
            A_STATIC_ASSERT_IS_POD(block_head);
            static_assert(sizeof(block_head) == page_head::reserved_size, "");
            static_assert(sizeof(block_head) == 32, "");
            SDL_TRACE_FUNCTION;
        }
    };
    static unit_test s_test;
}
#endif
}}} // sdl
