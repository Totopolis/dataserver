// page_memory.cpp
//
#include "dataserver/system/page_memory.h"

namespace sdl { namespace db { namespace memory {

PageMemory::PageMemory(const std::string & fname)
    : filename(fname)
{
    static_assert(page_size == 8 * 1024, "");
    static_assert(page_size == (1 << 13), ""); // 8192 = 2^13
}

page_head const *
PageMemory::load_page(pageIndex const i) const
{
    if (i.value() < m_pageCount) {
        SDL_ASSERT(0); // not implemented
        return nullptr;
    }
    SDL_TRACE("page not found: ", i.value());
    throw_error<PageMemory_error>("page not found");
    return nullptr;
}

} // memory
} // db
} // sdl
