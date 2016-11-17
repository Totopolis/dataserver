// algorithm.cpp
//
#include "dataserver/common/algorithm.h"

#if SDL_DEBUG
namespace sdl { namespace {
    class unit_test {
    public:
        unit_test()
        {
        }
    };
    static unit_test s_test;
}
} // sdl
#endif //#if SV_DEBUG

