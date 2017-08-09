// block_head.cpp
//
#include "dataserver/bpool/block_head.h"

namespace sdl { namespace db { namespace bpool {
namespace {
    A_STATIC_ASSERT_IS_POD(block_index);
    A_STATIC_ASSERT_IS_POD(block_head);
    static_assert(sizeof(block_index) == sizeof(uint32), "");
    static_assert(pool_limits::max_block * pool_limits::block_size == terabyte<1>::value, "");
    static_assert(sizeof(block_head) == page_head::reserved_size, "");
    static_assert(sizeof(block_head) == 32, "");
    static_assert(gigabyte<5>::value / pool_limits::block_size == 81920, "");
    static_assert(terabyte<1>::value == 1099511627776, "");
    static_assert(megabyte<1>::value == 1048576, "");
    static_assert(gigabyte<1>::value == 1073741824, "");
    static_assert(gigabyte<1>::value / 2 == pool_limits::block_size * 8 * 1024, "");
    static_assert(gigabyte<4>::value == 4294967296, "");
    static_assert(gigabyte<5>::value == 5368709120, "");
    static_assert(gigabyte<6>::value == 6442450944, "");
    static_assert(gigabyte<64>::value == 68719476736, "");
    static_assert(terabyte<1>::value / gigabyte<1>::value == 1024, "");
    static_assert(terabyte<1>::value / megabyte<1>::value == 1048576, "");
    static_assert(gigabyte<1>::value / megabyte<1>::value == 1024, "");
    static_assert(gigabyte<4>::value / megabyte<1>::value == 4096, "");
    static_assert(gigabyte<8>::value / megabyte<1>::value / 8 == 1024, "");     // *
    static_assert(terabyte<1>::value / gigabyte<1>::value / 8 == 128, "");      // 1 bit per 1 GB
    static_assert(terabyte<1>::value / megabyte<1>::value / 8 == 131072, "");   // 1 bit per 1 MB
    static_assert(gigabyte<64>::value / megabyte<1>::value / 8 == 8192, "");    // 1 bit per 1 MB *
    static_assert(terabyte<1>::value / gigabyte<8>::value == 128, "");          // *
    static_assert(megabyte<1>::value / pool_limits::block_size == 16, "");                // # blocks
    static_assert(gigabyte<1>::value / pool_limits::block_size == 16384, "");             // # blocks
    static_assert(terabyte<1>::value / pool_limits::block_size == 16777216, "");          // # blocks
    static_assert(gigabyte<6>::value / pool_limits::block_size == 98304, "");             // # blocks
    static_assert(gigabyte<5>::value / pool_limits::block_size == 81920, "");             // # blocks
    static_assert(gigabyte<6>::value / pool_limits::block_size / 2 / 8 == 6144, "");      // 1 bit per 2 blocks
    static_assert(terabyte<1>::value / pool_limits::block_size / 2 / 8 == 1048576, "");   // 1 bit per 2 blocks
    static_assert(terabyte<1>::value / pool_limits::block_size / 8 / 8 == 262144, "");    // 1 bit per 8 blocks
    static_assert(gigabyte<6>::value / pool_limits::block_size / 8 / 8 == 1536, "");      // 1 bit per 8 blocks
    static_assert(gigabyte<5>::value / pool_limits::block_size / 8 / 8 == 1280, "");      // 1 bit per 8 blocks
} // namespace

#if SDL_DEBUG
namespace {
struct uint24_8 { // 4 bytes
    unsigned int _24 : 24;
    unsigned int _8 : 8;
};
class unit_test {
public:
    unit_test() {
        {
            using T = pool_limits;
            static_assert(sizeof(uint24_8) == sizeof(uint32), "");
            block_index test{};
            test.d.blockId = T::last_block;
            SDL_ASSERT((test.value & block_index::blockIdMask) == test.d.blockId);
            SDL_ASSERT(T::last_block == test.value);
            SDL_ASSERT(T::last_block == test.d.blockId);
            SDL_ASSERT(!test.d.pageLock);
            for (size_t i = 0; i < T::block_page_num; ++i) {
                test.set_lock_page(i);
                SDL_ASSERT(((test.value & block_index::pageLockMask) >> 24) == test.d.pageLock);                    
                SDL_ASSERT(test.is_lock_page(i));
                SDL_ASSERT(test.d.pageLock == (1 << i));
                SDL_ASSERT(!test.clr_lock_page(i));
            }
            for (uint32 i = 0; i < T::max_block; ++i) {
                test.set_blockId(i);
                SDL_ASSERT(test.blockId() == i);
                SDL_ASSERT(!test.d.pageLock);
            }
            SDL_ASSERT(T::last_block == test.value);
        }
        SDL_TRACE_FUNCTION;
    }
};
static unit_test s_test;
}
#endif // SDL_DEBUG
}}} // sdl
