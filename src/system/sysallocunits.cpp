// sysallocunits.cpp
//
#include "common/common.h"
#include "sysallocunits.h"

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
                    SDL_TRACE(__FILE__);
                    A_STATIC_ASSERT_IS_POD(sysallocunits_row);
                    static_assert(sizeof(sysallocunits_row) < page_head::body_size, "");
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG