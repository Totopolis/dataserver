// page_memory.h
//
#pragma once
#ifndef __SDL_SYSTEM_PAGE_MEMORY_H__
#define __SDL_SYSTEM_PAGE_MEMORY_H__

#include "dataserver/system/page_head.h"

namespace sdl { namespace db { namespace memory {

class page_memory : noncopyable
{
    enum { page_size = page_head::page_size };
public:
    explicit page_memory(const std::string & fname)
    {
    }
    size_t page_count() const
    {
        return 0;
    }
    page_head const * load_page(pageIndex) const
    {
        return nullptr;
    }
};

} // memory
} // db
} // sdl

#endif // __SDL_SYSTEM_PAGE_MEMORY_H__
