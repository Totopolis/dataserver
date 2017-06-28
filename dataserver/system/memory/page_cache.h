// page_cache.h
//
#pragma once
#ifndef __SDL_SYSTEM_MEMORY_PAGE_CACHE_H__
#define __SDL_SYSTEM_MEMORY_PAGE_CACHE_H__

#include "dataserver/system/page_head.h"
#include "dataserver/spatial/interval_set.h"

namespace sdl { namespace db { namespace mmu {

class page_cache : noncopyable {
    using page32 = pageFileID::page32;
    using page_set = interval_set<page32>;
public:
    explicit page_cache(const std::string & fname);
    ~page_cache();
    bool is_open() const;
    void load_page(page32);
    void load_pages(page_set const &);
    void detach_file();
private:
    class data_t;
    const std::unique_ptr<data_t> data;
};

} // mmu
} // db
} // sdl

#endif // __SDL_SYSTEM_MEMORY_PAGE_CACHE_H__
