// page_map.h
//
#ifndef __SDL_SYSTEM_PAGE_MAP_H__
#define __SDL_SYSTEM_PAGE_MAP_H__

#pragma once

#include "page_head.h"
#include "filesys/file_map.h"

namespace sdl { namespace db {

class PageMapping : noncopyable
{
    using PageMapping_error = sdl_exception_t<PageMapping>;

    enum { page_size = page_head::page_size };
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
    page_head const * load_page(pageFileID const &) const;
private:
    size_t m_pageCount = 0;
    FileMapping m_fmap;
};

} // db
} // sdl

#endif // __SDL_SYSTEM_PAGE_MAP_H__
