// vm_alloc.cpp
//
#include "dataserver/system/memory/vm_alloc.h"
#include "dataserver/system/page_head.h"

namespace sdl { namespace db { namespace mmu {

#if SDL_DEBUG
#define SDL_DEBUG_SMALL_MEMORY  1
#else
#define SDL_DEBUG_SMALL_MEMORY  0
#endif

class vm_alloc::data_t: noncopyable {
    using this_error = sdl_exception_t<vm_alloc::data_t>;
public:
    enum { page_size = page_head::page_size }; // 8192 byte
    uint64 const byte_reserved;
    explicit data_t(uint64);
    ~data_t();
    void * alloc(uint64 start, uint64 size);
    void clear(uint64 start, uint64 size);
private:
#if SDL_DEBUG_SMALL_MEMORY
    using slot_t = std::unique_ptr<char[]>;
    std::vector<slot_t> slots;  // prototype for small memory only
#endif
};

vm_alloc::data_t::data_t(uint64 const size)
    : byte_reserved(size)
{
    SDL_ASSERT(size && !(size % page_size));
#if SDL_DEBUG_SMALL_MEMORY
    slots.resize(size / page_size);
#endif
}

vm_alloc::data_t::~data_t()
{
}

void * vm_alloc::data_t::alloc(uint64 const start, uint64 const size)
{
    SDL_ASSERT(start + size <= byte_reserved);
    SDL_ASSERT(size && !(size % page_size));
    SDL_ASSERT(!(start % page_size));
#if SDL_DEBUG_SMALL_MEMORY
    const size_t index = start / page_size;
    if (slots.size() < index) {
        throw_error<this_error>("bad alloc");
    }
    auto & p = slots[index];
    if (!p){
        p.reset(new char[page_size]);
    }
    return p.get();
#else
    return nullptr;
#endif
}

void vm_alloc::data_t::clear(uint64 const start, uint64 const size)
{
    SDL_ASSERT(start + size <= byte_reserved);
    SDL_ASSERT(size && !(size % page_size));
    SDL_ASSERT(!(start % page_size));
#if SDL_DEBUG_SMALL_MEMORY
    const size_t index = start / page_size;
    if (slots.size() < index) {
        throw_error<this_error>("bad page");
    }
    slots[index].reset();
#endif
}

//---------------------------------------------

vm_alloc::vm_alloc(uint64 const size)
    : data(new data_t(size))
{
}

vm_alloc::~vm_alloc()
{
}

uint64 vm_alloc::reserved() const
{
    return data->byte_reserved;
}

void * vm_alloc::alloc(uint64 const start, uint64 const size)
{
    return data->alloc(start, size);
}

void vm_alloc::clear(uint64 const start, uint64 const size)
{
    data->clear(start, size);
}

#if SDL_DEBUG
namespace {
    class unit_test {
    public:
        unit_test() {
            if (1) {
                enum { page_size = page_head::page_size };
                vm_alloc test(page_size);
                SDL_WARNING(test.alloc(0, page_size));
                test.clear(0, page_size);
            }
        }
    };
    static unit_test s_test;
}
#endif //#if SDL_DEBUG

} // mmu
} // sdl
} // db

