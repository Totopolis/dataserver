// syscolpars.cpp
//
#include "common/common.h"
#include "syscolpars.h"

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
                    A_STATIC_ASSERT_IS_POD(syscolpars);
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG