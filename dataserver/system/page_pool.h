// page_pool.h
//
#pragma once
#ifndef __SDL_SYSTEM_PAGE_POOL_H__
#define __SDL_SYSTEM_PAGE_POOL_H__

#include "dataserver/system/page_pool_file.h"
#if SDL_TEST_PAGE_POOL
#include "dataserver/spatial/sparse_set.h"
#if defined(SDL_OS_WIN32)
#include "dataserver/system/vm_win32.h"
#else
#include "dataserver/system/vm_unix.h"
#endif
namespace sdl { namespace db { namespace pp {

#if defined(SDL_OS_WIN32) && SDL_DEBUG
#define SDL_PAGE_POOL_STAT          0  // statistics
#define SDL_PAGE_POOL_LOAD_ALL      0  // must be off
#else
#define SDL_PAGE_POOL_STAT          0  // statistics
#define SDL_PAGE_POOL_LOAD_ALL      0  // must be off
#endif

class BasePool : noncopyable {
public:
    enum { slot_page_num = 8 };                         // 1 extent
    enum { page_size = page_head::page_size };          // 8 KB = 8192 byte = 2^13
    enum { slot_size = page_size * slot_page_num };     // 64 KB = 65536 byte = 2^16
    static constexpr size_t max_page = size_t(1) << 32;             // 4,294,967,296
    static constexpr size_t max_slot = max_page / slot_page_num;    // 536,870,912
protected:
    explicit BasePool(const std::string & fname);
private:
    static bool valid_filesize(size_t);
protected:
    PagePoolFile m_file;
};

class PagePool final : BasePool {
    using this_error = sdl_exception_t<PagePool>;
    using lock_guard = std::lock_guard<std::mutex>;
    enum { commit_all = 1 };
public:
    explicit PagePool(const std::string & fname);
    bool is_open() const {
        return m_alloc.is_open();
    }
    size_t filesize() const {
        return m.filesize;
    }
    size_t page_count() const {
        return m.page_count;
    }
    size_t slot_count() const {
        return m.slot_count;
    }
    void const * start_address() const {
        return m_alloc.base_address();
    }
    page_head const * load_page(pageIndex);

#if SDL_PAGE_POOL_STAT
    struct page_stat_t {
        sparse_set<uint32> load_page;
        sparse_set<uint32> load_slot;
        size_t load_page_request = 0;
        void trace() const;
    };
    using unique_page_stat = std::unique_ptr<page_stat_t>;
    thread_local static unique_page_stat thread_page_stat;
#endif
private:
#if SDL_DEBUG
    static bool check_page(page_head const *, pageIndex);
#endif
    void load_all();
    page_head const * load_page_nolock(pageIndex);
private:
    struct info_t {
        size_t const filesize = 0;
        size_t const page_count = 0;
        size_t const slot_count = 0;
        size_t last_slot = 0;
        size_t last_slot_page_count = 0;
        size_t last_slot_size = 0;
        explicit info_t(size_t);
    };
    size_t alloc_slot_size(const size_t slot) const {
        SDL_ASSERT(slot < m.slot_count);
        if (slot == m.last_slot)
            return m.last_slot_size;
        return slot_size;
    }
private:
    const info_t m; // read-only
    std::mutex m_mutex; // will improve
#if defined(SDL_OS_WIN32)
    vm_win32 m_alloc;
#else
    vm_unix m_alloc;
#endif
    std::vector<bool> m_slot_commit; //FIXME: spin lock to check m_slot_commit
};

} // pp
} // db
} // sdl

#endif // SDL_TEST_PAGE_POOL
#endif // __SDL_SYSTEM_PAGE_POOL_H__
