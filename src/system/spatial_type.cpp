// spatial_type.cpp
//
#include "common/common.h"
#include "spatial_type.h"

namespace sdl { namespace db { 

spatial_point spatial_transform::make_point(spatial_cell const & d, spatial_grid const & g)
{
    return {};
}

spatial_cell spatial_transform::make_cell(spatial_point const & d, spatial_grid const & g)
{
    //
    return {};
}

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
                    A_STATIC_ASSERT_IS_POD(spatial_cell);
                    A_STATIC_ASSERT_IS_POD(spatial_point);
                    static_assert(sizeof(spatial_cell) == 5, "");
                    static_assert(sizeof(spatial_point) == 16, "");
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG
