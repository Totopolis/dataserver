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
                    using index_page_row = index_page_row_t<uint64>;
                    A_STATIC_ASSERT_IS_POD(index_page_row);
                    static_assert(sizeof(index_page_row) == 15, "");
                    static_assert(sizeof(index_page_row) == (7 + sizeof(index_page_row::key_type)), "");
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG