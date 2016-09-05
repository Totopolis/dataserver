// static.cpp
//
#include "common.h"
#include "static.h"

namespace sdl {

} // sdl

#if SDL_DEBUG
namespace sdl { namespace {
    class unit_test {
    public:
        unit_test() {
            static_assert(is_odd(1), "");
            static_assert(is_odd(-1), "");
            static_assert(!is_odd(2), "");
            static_assert(!is_odd(-2), "");
            static_assert(is_pod(0), "");
        }
    };
    static unit_test s_test;
}} // sdl
#endif //#if SV_DEBUG
