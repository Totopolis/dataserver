// memory.h
//
#pragma once
#ifndef __SDL_SYSTEM_MEMORY_H__
#define __SDL_SYSTEM_MEMORY_H__

#include "dataserver/system/page_head.h"

namespace sdl { namespace db { namespace memory {

class PageMemory : noncopyable {
public:
    const std::string filename;
    explicit PageMemory(const std::string & fname);
    size_t page_count() const {
        return m_pageCount;
    }
    page_head const * load_page(pageIndex) const;
private:
    using PageMemory_error = sdl_exception_t<PageMemory>;
    enum { page_size = page_head::page_size };
    size_t m_pageCount = 0;
};

} // memory
} // db
} // sdl

#endif // __SDL_SYSTEM_MEMORY_H__
