// spatial_type.cpp
//
#include "common/common.h"
#include "spatial_type.h"

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
                    static_assert(sizeof(spatial_cell) == 5, "");
                    static_assert(sizeof(spatial_point) == 16, "");
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG
