// maketable_meta.cpp
//
#include "common/common.h"
#include "maketable_meta.h"

namespace sdl { namespace db { namespace make { namespace meta {

#if SDL_DEBUG
std::string trace_type::short_name(const char * const s) {
    static const char remove[] = "sdl::db::make::";
    std::string name(s);
    for (;;) {
        const auto pos = name.find(remove);
        if (pos != std::string::npos) {
            name.erase(pos, A_ARRAY_SIZE(remove)-1);
        }
        else
            break;
    }
    return name;
}
#endif

} // meta
} // make
} // db
} // sdl
