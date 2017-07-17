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
            A_STATIC_ASSERT_IS_POD(block_head);
            using T = pool_limits;
            static_assert(sizeof(block_index) == sizeof(uint32), "");
            static_assert(T::max_block * T::block_size == terabyte<1>::value, "");
            static_assert(sizeof(block_head) == page_head::reserved_size, "");
            static_assert(sizeof(block_head) == 32, "");
            static_assert(gigabyte<5>::value / T::block_size == 81920, "");
            static_assert(gigabyte<1>::value == 1073741824, "");
            static_assert(gigabyte<4>::value == 4294967296, "");
            static_assert(gigabyte<5>::value == 5368709120, "");
            static_assert(gigabyte<6>::value == 6442450944, "");
            static_assert(terabyte<1>::value / T::block_size == 16777216, "");          // # blocks per 1 TB
            static_assert(gigabyte<6>::value / T::block_size == 98304, "");             // # blocks per 6 GB
            static_assert(gigabyte<5>::value / T::block_size == 81920, "");             // # blocks per 5 GB
            static_assert(gigabyte<6>::value / T::block_size / 2 / 8 == 6144, "");      // 1 bit per 2 blocks per 6 GB space
            static_assert(terabyte<1>::value / T::block_size / 2 / 8 == 1048576, "");   // 1 bit per 2 blocks per 6 GB space
            static_assert(terabyte<1>::value / T::block_size / 8 / 8 == 262144, "");    // 1 bit per 8 blocks per 1 TB space
            static_assert(gigabyte<6>::value / T::block_size / 8 / 8 == 1536, "");      // 1 bit per 8 blocks per 6 GB space
            static_assert(gigabyte<5>::value / T::block_size / 8 / 8 == 1280, "");      // 1 bit per 8 blocks per 5 GB space
            {
                block_index test{};
                test.d.offset = T::last_block;
                SDL_ASSERT(T::last_block == test.value);
                SDL_ASSERT(T::last_block == test.d.offset);
                SDL_ASSERT(!test.d.pageLock);
                for (size_t i = 0; i < T::block_page_num; ++i) {
                    test.set_lock_page(i);
                    SDL_ASSERT(test.is_lock_page(i));
                    SDL_ASSERT(test.d.pageLock == (1 << i));
                    test.clr_lock_page(i);
                }
                for (uint32 i = 0; i < T::max_block; ++i) {
                    test.set_offset(i);
                    SDL_ASSERT(test.offset() == i);
                    SDL_ASSERT(!test.d.pageLock);
                }
                SDL_ASSERT(T::last_block == test.value);
            }
            {
            }
            SDL_TRACE_FUNCTION;
        }
    };
    static unit_test s_test;
}
#endif
}}} // sdl
