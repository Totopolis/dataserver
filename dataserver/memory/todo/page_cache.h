// page_cache.h
//
#pragma once
#ifndef __SDL_MEMORY_PAGE_CACHE_H__
#define __SDL_MEMORY_PAGE_CACHE_H__

#include "dataserver/system/page_head.h"

namespace sdl { namespace db { namespace mmu {

class page_cache : noncopyable {
    using page32 = pageFileID::page32;
public:
    explicit page_cache(const std::string & fname);
    ~page_cache();
    bool is_open() const;
    void load_page(page32);
    void detach_file();
private:
    template<class T>
    void load_pages(T const &); // reserved
private:
    class data_t;
    const std::unique_ptr<data_t> data;
};

template<class T>
void page_cache::load_pages(T const & s) {
    for (const auto & id : s) {
        A_STATIC_CHECK_TYPE(page32 const &, id);
        load_page(id);
    }
}

} // mmu
} // db
} // sdl

#endif // __SDL_MEMORY_PAGE_CACHE_H__
