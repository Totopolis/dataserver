// spatial_tree.cpp
//
#include "common/common.h"
#include "spatial_tree.h"

#if SDL_DEBUG
namespace sdl { namespace db { namespace {
class unit_test {
public:
    unit_test()
    {
        if (0) {
            spatial_tree test(nullptr, nullptr, nullptr, nullptr);
            test.for_point(spatial_point{},[](spatial_page_row const *){
                return true;
            });
        }
    }
};
static unit_test s_test;
}
} // db
} // sdl
#endif //#if SV_DEBUG