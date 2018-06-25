// version_t.cpp
#include "dataserver/common/version_t.h"
#include "dataserver/common/format.h"
#include "dataserver/common/version.h"

namespace sdl {

std::string version_t::format() const {
    static const char mask[] = "%d.%d.%d";
    char buf[64] = {};
    return sdl::format_s(buf, mask, (int)major, (int)minor, (int)revision);
}

version_t version_t::parse(const char * const s) {
    SDL_ASSERT(sdl::is_str_valid(s));
    version_t result {};
    if (sdl::is_str_valid(s)) {
        const char * p = s; 
        const char * const end = p + strlen(s);
        const char * next = std::find(p, end, '.');
        if (next != end) {
            result.major = ::atoi(p);
            p = next + 1;
            next = std::find(p, end, '.');
            if (next != end) {
                result.minor = ::atoi(p);
                result.revision = ::atoi(next + 1);
            }
        }
    }
    return result;
}

} // sdl

#if SDL_DEBUG
namespace sdl { namespace {
    class unit_test {
    public:
        unit_test() {
            SDL_TRACE(version_t::current());
            const auto t = version_t::parse(SDL_DATASERVER_VERSION);
            SDL_ASSERT(t.major == SDL_DATASERVER_VERSION_MAJOR);
            SDL_ASSERT(t.minor == SDL_DATASERVER_VERSION_MINOR);
            SDL_ASSERT(t.revision == SDL_DATASERVER_VERSION_REVISION);
        }
    };
    static unit_test s_test;
}
} // sdl
#endif //#if SDL_DEBUG
