// page_map.cpp
//
#include "common/common.h"
#include "page_map.h"

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

page_head const *
PageMapping::load_page(pageIndex const i) const
{
    const size_t pageIndex = i.value();
    if (pageIndex < m_pageCount) {
        const char * const data = static_cast<const char *>(m_fmap.GetFileView());
        const char * p = data + pageIndex * page_size;
        return reinterpret_cast<page_head const *>(p);
    }
    //SDL_TRACE_4("\n*** load_page failed: ", pageIndex, " of ", m_pageCount);
    //SDL_WARNING(0);
    return nullptr;
}

page_head const *
PageMapping::load_page(pageFileID const & id) const
{
    if (id.is_null()) {
        return nullptr;
    }
    return load_page(pageIndex(id.pageId));
}

} // db
} // sdl
