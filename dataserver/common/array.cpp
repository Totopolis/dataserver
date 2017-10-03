// array.cpp
//
#include "dataserver/common/array.h"


#if SDL_DEBUG
namespace sdl { namespace {
    class unit_test {
    public:
        unit_test() {
            {
                using T = array_t<int, 10>;
                static_assert(sizeof(T) == sizeof(T::elems_t), "");
                A_STATIC_ASSERT_IS_POD(T);
            }
            {
                using T = array_t<std::string, 10>;
                static_assert(sizeof(T) == sizeof(T::elems_t), "");
            }
        }
    };
    static unit_test s_test;
}
} // sdl
#endif //#if SDL_DEBUG