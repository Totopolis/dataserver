// bitmap_cell.cpp
//
#include "common/common.h"
#include "bitmap_cell.h"
#include <unordered_map>

namespace sdl { namespace db {

struct bitmap_cell {
   
#pragma pack(push, 1) 

    struct mask_256 {
        uint32 _32[4];
    };

    struct node_type {
        mask_256 cell_id;
        uint32 next;
    };

#pragma pack(pop)

    struct allocator {
        using map_type = std::unordered_map<uint32, uint32>;
        using vector_node = std::vector<node_type>;
    };

};

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
                    static_assert(sizeof(bitmap_cell::mask_256) == 16, "");
                    static_assert(sizeof(bitmap_cell::node_type) == 20, "");
                    A_STATIC_ASSERT_IS_POD(bitmap_cell::mask_256);
                    A_STATIC_ASSERT_IS_POD(bitmap_cell::node_type);
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG
