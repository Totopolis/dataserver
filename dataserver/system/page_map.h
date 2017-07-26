// page_map.h
//
#pragma once
#ifndef __SDL_SYSTEM_PAGE_MAP_H__
#define __SDL_SYSTEM_PAGE_MAP_H__

#include "dataserver/system/page_head.h"
#include "dataserver/filesys/file_map.h"

namespace sdl { namespace db {

class PageMapping : noncopyable {
    enum { page_size = page_head::page_size };
public:
    explicit PageMapping(const std::string & fname);
    bool is_open() const {
        return m_fmap.IsFileMapped();
    }
    void const * start_address() const {
        return m_fmap.GetFileView();
    }
    size_t page_count() const {
        return m_pageCount;
    }
    page_head const * lock_page(pageIndex) const; // load_page
    bool unlock_page(pageIndex) const;
    static void unlock_thread(bool){}
private:
    using PageMapping_error = sdl_exception_t<PageMapping>;
    size_t m_pageCount = 0;
    FileMapping m_fmap;
};

inline page_head const *
PageMapping::lock_page(pageIndex const i) const {
    const size_t page = i.value(); // uint32 => size_t
    if (page < m_pageCount) {
        const char * const data = static_cast<const char *>(start_address());
        return reinterpret_cast<page_head const *>(data + page * page_size);
    }
    SDL_TRACE("page not found: ", page);
    throw_error<PageMapping_error>("page not found");
    return nullptr;
}

inline bool PageMapping::unlock_page(pageIndex const id) const {
    SDL_ASSERT(id.value() < m_pageCount);
    return false;
}

} // db
} // sdl

#endif // __SDL_SYSTEM_PAGE_MAP_H__
