// vm_alloc.cpp
//
#include "dataserver/system/memory/vm_alloc.h"
#include "dataserver/system/page_head.h"

#if defined(SDL_OS_WIN32)
#endif

namespace sdl { namespace db { namespace mmu {

class vm_alloc::data_t: noncopyable {
public:
    enum { page_size = page_head::page_size }; // 8192 byte
    uint64 const m_reserved;
    explicit data_t(uint64);
    ~data_t();
    void * alloc(uint64 const start, uint64 const size);
    void clear();
};

vm_alloc::data_t::data_t(uint64 const size)
    : m_reserved(size)
{
    SDL_ASSERT(size && !(size % page_size));
}

vm_alloc::data_t::~data_t()
{
    clear();
}

void * vm_alloc::data_t::alloc(uint64 const start, uint64 const size)
{
    SDL_ASSERT(start + size <= m_reserved);
    SDL_ASSERT(size && !(size % page_size));
    SDL_ASSERT(!(start % page_size));
    return nullptr;
}

void vm_alloc::data_t::clear()
{
}

vm_alloc::vm_alloc(uint64 const size)
    : data(new data_t(size))
{
}

vm_alloc::~vm_alloc()
{
}

uint64 vm_alloc::reserved() const
{
    return data->m_reserved;
}

void * vm_alloc::alloc(uint64 const start, uint64 const size)
{
    return data->alloc(start, size);
}

void vm_alloc::clear()
{
    data->clear();
}

#if SDL_DEBUG
namespace {
    class unit_test {
    public:
        unit_test() {
            if (1) {
                vm_alloc test(page_head::page_size);
                if (!test.alloc(0, test.reserved())) {
                    SDL_WARNING(0);
                }
            }
        }
    };
    static unit_test s_test;
}
#endif //#if SDL_DEBUG

} // mmu
} // sdl
} // db

