// index_page.cpp
//
#include "common/common.h"
#include "index_page.h"
#include "page_info.h"

namespace sdl { namespace db { 

} // db
} // sdl

#if SDL_DEBUG
namespace sdl {
    namespace db {
        namespace {
            class unit_test {
            public:
                unit_test()
                {
                    SDL_TRACE_FILE;
                    A_STATIC_ASSERT_IS_POD(index_page_row_t<char>);
                    static_assert(sizeof(index_page_row_t<char>) == 7+1, "");
                    A_STATIC_ASSERT_IS_POD(index_page_row_t<uint64>);
                    A_STATIC_ASSERT_IS_POD(pair_key<uint64>);
                    static_assert(sizeof(index_page_row_t<uint32>) == 7+4, "");
                    static_assert(sizeof(index_page_row_t<uint64>) == 7+8, "");
                    static_assert(sizeof(index_page_row_t<guid_t>) == 7+16, "");
                    static_assert(sizeof(pair_key<uint64>) == 16, "");
                    static_assert(index_row_head_size == 7, "");
                    A_STATIC_ASSERT_TYPE(int32, index_key<scalartype::t_int>);
                    static_assert(index_key_t<scalartype::t_int>::value == scalartype::t_int, "");
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG