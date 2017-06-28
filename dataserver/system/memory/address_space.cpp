// address_space.cpp
//
#include "dataserver/system/memory/address_space.h"

#if defined(SDL_OS_WIN32)
#endif

namespace sdl { namespace db { namespace mmu {

vm_alloc::vm_alloc(uint64 const size)
    : m_reserved(size)
{
    SDL_ASSERT(size && !(size % page_size));
}

vm_alloc::~vm_alloc()
{
}

void * vm_alloc::alloc(uint64 const start, uint64 const size)
{
    SDL_ASSERT(start + size <= m_reserved);
    SDL_ASSERT(size && !(size % page_size));
    SDL_ASSERT(!(start % page_size));
    return nullptr;
}

#if SDL_DEBUG
namespace {
    class unit_test {
    public:
        unit_test() {
            using T = address_translation;
            if (1) {
                T test;
                SDL_ASSERT(test[0] == 0);
                SDL_ASSERT(test[test.size() - 1] == 0);
                test.page(0) = 1;
                SDL_ASSERT(test[0] == 1);
            }
            if (1) {
                vm_alloc test(page_head::page_size);
                auto p = test.alloc(0, test.reserved());
            }
        }
    };
    static unit_test s_test;
}
#endif //#if SDL_DEBUG

} // mmu
} // sdl
} // db

