// algorithm.cpp
//
#include "dataserver/common/algorithm.h"

#if SDL_DEBUG
namespace sdl { namespace algo { namespace {
    class unit_test {
    public:
        unit_test()
        {
            SDL_ASSERT(number_of_1(0x1) == 1);
            SDL_ASSERT(number_of_1(0x101) == 2);
            SDL_ASSERT(number_of_1(0x111) == 3);
            SDL_ASSERT(number_of_1<uint8>(0x80) == 1);
            SDL_ASSERT(number_of_1<uint8>(0x88) == 2);
            SDL_ASSERT(number_of_1<int16>(-1) == 16);
            SDL_ASSERT(number_of_1<int32>(-1) == 32);
            SDL_ASSERT(number_of_1<int64>(-1) == 64);
        }
    };
    static unit_test s_test;
}
} // algo
} // sdl
#endif //#if SV_DEBUG

