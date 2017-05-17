// pfs_page.cpp
//
#include "dataserver/sysobj/pfs_page.h"
#include "dataserver/system/page_info.h"

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
                    A_STATIC_ASSERT_IS_POD(pfs_page_row);
                    static_assert(pfs_page_row::pfs_size * page_head::page_size == 8088 * 8192, "");
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SDL_DEBUG
