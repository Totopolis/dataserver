// page_map.h
//
#pragma once
#ifndef __SDL_SYSTEM_PAGE_MAP_H__
#define __SDL_SYSTEM_PAGE_MAP_H__

#include "dataserver/system/page_head.h"
#include "dataserver/filesys/file_map.h"

namespace sdl { namespace db {

class PageMapping : noncopyable {
public:
    const std::string filename;
    explicit PageMapping(const std::string & fname);
    bool is_open() const
    {
        return m_fmap.IsFileMapped();
    }
    void const * start_address() const
    {
        return m_fmap.GetFileView();
    }
    size_t page_count() const
    {   
        return m_pageCount;
    }
    page_head const * load_page(pageIndex) const;
private:
    using PageMapping_error = sdl_exception_t<PageMapping>;
    enum { page_size = page_head::page_size };
    size_t m_pageCount = 0;
    FileMapping m_fmap;
};

inline page_head const *
PageMapping::load_page(pageIndex const i) const {
    if (i.value() < m_pageCount) {
        const char * const data = static_cast<const char *>(m_fmap.GetFileView());
        return reinterpret_cast<page_head const *>(data + i.value() * page_size);
    }
    SDL_TRACE("page not found: ", i.value());
    throw_error<PageMapping_error>("page not found");
    return nullptr;
}

} // db
} // sdl

#endif // __SDL_SYSTEM_PAGE_MAP_H__
