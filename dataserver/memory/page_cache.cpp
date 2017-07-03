// page_cache.cpp
//
#include "dataserver/memory/page_cache.h"
#include "dataserver/memory/address_table.h"
#include "dataserver/memory/vm_alloc.h"

namespace sdl { namespace db { namespace mmu {

class page_cache::data_t : noncopyable {
public:
    address_table tbl;
    std::unique_ptr<vm_alloc> alloc;
    explicit data_t(const std::string & fname){}
};

page_cache::page_cache(const std::string & fname)
    : data(new data_t(fname))
{
}

page_cache::~page_cache()
{
}

bool page_cache::is_open() const
{
    return false;
}

void page_cache::detach_file()
{
}

//todo: load 8 kilobyte page from file into memory buffer
void page_cache::load_page(page32 const id)
{
    SDL_ASSERT(0);
}

#if SDL_DEBUG
namespace {
    class unit_test {
    public:
        unit_test() {
            if (0) {
                page_cache test("");
                test.load_page(0);
            }
        }
    };
    static unit_test s_test;
}
#endif //#if SDL_DEBUG

} // mmu
} // sdl
} // db

