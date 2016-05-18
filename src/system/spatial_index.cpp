// spatial_index.cpp
//
#include "common/common.h"
#include "spatial_index.h"

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
                    static_assert(sizeof(todo::spatial_cell) == 5, "");
                    static_assert(sizeof(todo::spatial_root_row) == 20, "");
                    static_assert(sizeof(todo::spatial_node_row) == 23, "");
                    static_assert(sizeof(todo::spatial_leaf_row) == 26, "");
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG