// thread.cpp
#include "dataserver/common/thread.h"

namespace sdl {

#if SDL_DEBUG
namespace {
    struct unit_test {
        unit_test() {
        }
    };
    static unit_test s_test;
}
#endif // SDL_DEBUG

} // sdl