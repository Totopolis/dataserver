// config.cpp
//
#include "common.h"
#include "config.h"

#if SDL_DEBUG
namespace sdl {
int debug::warning_level = 1;
void debug::warning(const char * message, const char * fun, const int line) {
    if (warning_level) {
        std::cout << "\nwarning (" << message << ") in " << fun << " at line " << line << std::endl; 
        SDL_ASSERT(warning_level < 2);
    }
}
} // sdl
#endif
