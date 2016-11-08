// config.cpp
//
#include "dataserver/common/config.h"

namespace sdl {
int debug::warning_level = 1;
int debug::unit_test = 0;
#if SDL_TRACE_ENABLED
void debug::warning(const char * message, const char * fun, const int line) {
    if (warning_level) {
        std::cout << "\nwarning (" << message << ") in " << fun << " at line " << line << std::endl; 
        SDL_ASSERT(warning_level < 2);
    }
}
#endif
} // sdl

