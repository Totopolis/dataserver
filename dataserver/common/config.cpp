// config.cpp
//
#include "dataserver/common/config.h"

#if SDL_DEBUG || defined(SDL_TRACE_RELEASE)
namespace sdl {

int & debug::warning_level() { 
    static int value = 1; 
    return value;
}

bool & debug::is_unit_test() {
    static bool value = false;
    return value;
}

bool & debug::is_print_timestamp() {
    static bool value = false;
    return value;
}

void debug::print_timestamp() {
    if (is_print_timestamp()) {
        std::cout << static_cast<unsigned int>(std::time(nullptr)) << "> ";
    }
}

void debug::print_timestampw() {
    if (is_print_timestamp()) {
        std::wcout << static_cast<unsigned int>(std::time(nullptr)) << L"> ";
    }
}

void debug::trace() {
    std::cout << std::endl;
}

void debug::trace_with_timestamp() {
    print_timestamp();
    trace();
}

void debug::tracew_with_timestamp() {
    print_timestampw();
    tracew();
}

void debug::warning(const char * message, const char * fun, const int line) {
    if (warning_level() && message && fun) {
        std::cout << "\n";
        print_timestamp();
        std::cout << "warning (" << message << ") in " << fun << " at line " << line << std::endl; 
        assert(warning_level() < 2);
    }
}

namespace {
class unit_test {
public:
    unit_test() {
        if (0) {
            SDL_TRACE(nullptr);
            SDL_TRACEW(nullptr);
        }
    }
};
static unit_test s_test;
}} // sdl
#endif // SDL_DEBUG


