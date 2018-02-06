// cyrillic.cpp
#include "dataserver/utils/cyrillic.h"

#if SDL_DEBUG
namespace sdl { namespace utils { namespace {

class unit_test {
public:
    unit_test() {
        for (wchar_t i = cyrillic::Cyr_A; i <= cyrillic::Cyr_JA; ++i) {
            const auto w = cyrillic::to_lower(i);
            SDL_ASSERT(cyrillic::to_upper(w) == i);
        }
        for (wchar_t i = cyrillic::Cyr_a; i <= cyrillic::Cyr_ja; ++i) {
            const auto w = cyrillic::to_upper(i);
            SDL_ASSERT(cyrillic::to_lower(w) == i);
        }
    }
};
static unit_test s_test;

}}} // sdl
#endif //#if SDL_DEBUG
