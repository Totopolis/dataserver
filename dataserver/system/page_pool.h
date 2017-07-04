// page_pool.h
//
#pragma once
#ifndef __SDL_SYSTEM_PAGE_POOL_H__
#define __SDL_SYSTEM_PAGE_POOL_H__

#include "dataserver/system/page_head.h"

#if defined(SDL_OS_WIN32) && (SDL_DEBUG > 1)
#define SDL_TEST_PAGE_POOL   1  // experimental
#define SDL_PAGE_POOL_STAT   1  // statistics
#else
#define SDL_TEST_PAGE_POOL   0
#endif

#if SDL_TEST_PAGE_POOL
#include <fstream>
#include <set>
#endif

#if SDL_TEST_PAGE_POOL
namespace sdl { namespace db { namespace pp {

class PagePool : noncopyable {
    using this_error = sdl_exception_t<PagePool>;
    using lock_guard = std::lock_guard<std::mutex>;
    enum { page_size = page_head::page_size };
public:
    explicit PagePool(const std::string & fname);
    bool is_open() const {
        return !!m_alloc;
    }
    size_t filesize() const {
        return m_filesize;
    }
    size_t page_count() const {
        return m_page_count;
    }
    void const * start_address() const {
        return m_alloc.get();
    }
    page_head const * load_page(pageIndex);

#if SDL_PAGE_POOL_STAT
    struct page_stat_t {
        std::set<size_t> load_page;
    };
    using unique_page_stat = std::unique_ptr<page_stat_t>;
    thread_local static unique_page_stat thread_page_stat;
#endif
private:
    static bool assert_page(page_head const * const head, const size_t pageId) {
        SDL_ASSERT(head->valid_checksum() || !head->data.tornBits);
        SDL_ASSERT(head->data.pageId.pageId == pageId);
        return true;
    }
    void load_all();
private:
    std::mutex m_mutex;
    std::ifstream m_file;
    std::vector<bool> m_commit;
    std::unique_ptr<char[]> m_alloc; // huge memory
    size_t m_filesize = 0;
    size_t m_page_count = 0;
    size_t m_page_loaded = 0;
};

} // pp
} // db
} // sdl

#endif // SDL_TEST_PAGE_POOL
#endif // __SDL_SYSTEM_PAGE_POOL_H__
