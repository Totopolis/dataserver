// address_tbl.cpp
//
#include "dataserver/memory/address_tbl.h"

namespace sdl { namespace db { namespace mmu {

#if SDL_DEBUG
namespace {
    class unit_test {
    public:
        unit_test() {
            if (1) {
                using T = address_tbl;
                T test;
                SDL_ASSERT(test[0] == 0);
                SDL_ASSERT(test[test.size() - 1] == 0);
            }
        }
    };
    static unit_test s_test;
}
#endif //#if SDL_DEBUG

} // mmu
} // sdl
} // db

