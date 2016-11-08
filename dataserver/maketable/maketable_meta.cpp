// maketable_meta.cpp
//
#include "dataserver/maketable/maketable_meta.h"

namespace sdl { namespace db { namespace make { namespace meta {

#if SDL_DEBUG
namespace {
    size_t find(std::string const & s, const char * const token, size_t const pos, size_t const end) {
        const auto p = s.find(token, pos);
        if (end < p) {
            return std::string::npos;
        }
        return p;
    }
}
std::string trace_type::short_name(const char * const s) {
    SDL_ASSERT(s && s[0]);
    std::string name(s);
    size_t space = name.find(' ');    
    while (space != std::string::npos) {
        const size_t next_space = name.find(' ', space + 1);
        size_t p = find(name, "::", space + 1, next_space);
        while (p != std::string::npos) {
            SDL_ASSERT(p < next_space);
            size_t const p2 = find(name, "::", p + 2, next_space);
            if (p2 != std::string::npos) {
                p = p2;
            }
            else {
                break;
            }
        }
        if (p != std::string::npos) {
            SDL_ASSERT(space < p);
            const size_t len = p - space + 1;
            name.erase(space + 1, len);
            if (next_space != std::string::npos) {
                SDL_ASSERT(space + len < next_space);
                space = next_space - len;
                SDL_ASSERT(name[space] == ' ');
            }
            else {
                space = next_space;
            }
        }
        else {
            space = next_space;
        }
    }
    return name;
}
#endif // SDL_DEBUG

} // meta
} // make
} // db
} // sdl
