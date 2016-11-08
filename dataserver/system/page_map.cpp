// page_map.cpp
//
#include "dataserver/common/common.h"
#include "dataserver/system/page_map.h"

namespace sdl { namespace db {

PageMapping::PageMapping(const std::string & fname)
    : filename(fname)
{
    static_assert(page_size == 8 * 1024, "");
    if (m_fmap.CreateMapView(fname.c_str())) {
        const uint64 sz = m_fmap.GetFileSize();
        const uint64 pp = sz / page_size;
        SDL_ASSERT(!(sz % page_size));
        SDL_ASSERT(pp < size_t(-1));
        throw_error_if<PageMapping_error>((sz % page_size)!=0, "bad file size");
        m_pageCount = static_cast<size_t>(pp);
    }
    else {
        SDL_WARNING(false);
        m_pageCount = 0;
    }
    throw_error_if<PageMapping_error>(!m_pageCount, "empty file");
}

} // db
} // sdl
