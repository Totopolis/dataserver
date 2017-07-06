// page_pool.h
//
#pragma once
#ifndef __SDL_SYSTEM_PAGE_POOL_H__
#define __SDL_SYSTEM_PAGE_POOL_H__

#include "dataserver/system/page_head.h"

#if defined(SDL_OS_WIN32) //&& (SDL_DEBUG > 1)
#define SDL_TEST_PAGE_POOL          1  // experimental
#define SDL_PAGE_POOL_STAT          1  // statistics
#define SDL_PAGE_POOL_LOAD_ALL      0  // must be off
#else
#define SDL_TEST_PAGE_POOL          0
#endif

#if SDL_TEST_PAGE_POOL
#include "dataserver/spatial/interval_set.h"
#include "dataserver/spatial/sparse_set.h"
#include <fstream>
#include <set>
#if defined(SDL_OS_WIN32)
#include <windows.h>
#endif
#endif

#if SDL_TEST_PAGE_POOL
namespace sdl { namespace db { namespace pp {

class PagePoolFile_s : noncopyable {
public:
    explicit PagePoolFile_s(const std::string & fname);
    size_t filesize() const { 
        return m_filesize;
    }
    bool is_open() const;
    void read_all(char * dest);
    void read(char * dest, size_t offset, size_t size);
private:
    size_t m_filesize = 0;
    std::ifstream m_file;
};

#if defined(SDL_OS_WIN32)
class PagePoolFile_win32 : noncopyable {
public:
    explicit PagePoolFile_win32(const std::string & fname);
    ~PagePoolFile_win32();
    size_t filesize() const { 
        return m_filesize;
    }
    bool is_open() const;
    void read_all(char * dest);
    void read(char * dest, size_t offset, size_t size);
private:
    size_t seek_beg(size_t offset);
    size_t seek_end();
private:
    size_t m_filesize = 0;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    SDL_DEBUG_CODE(size_t m_seekpos = 0;)
};
#endif // SDL_OS_WIN32

#if defined(SDL_OS_WIN32)
using PagePoolFile = PagePoolFile_win32;
#else
using PagePoolFile = PagePoolFile_s;
#endif // SDL_OS_WIN32

class PagePool : noncopyable {
    using this_error = sdl_exception_t<PagePool>;
    using lock_guard = std::lock_guard<std::mutex>;
    static constexpr size_t max_page = size_t(1) << 32; // 4,294,967,296
    static constexpr size_t max_slot = max_page / 8;    // 536,870,912
    enum { slot_page_num = 8 };                         // 1 extent
    enum { page_size = page_head::page_size };          // 8 KB = 8192 byte = 2^13
    enum { slot_size = page_size * slot_page_num };     // 64 KB = 65536 byte = 2^16 (or block_size)
public:
    explicit PagePool(const std::string & fname);
    bool is_open() const {
        return !!m_alloc;
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
        return m_alloc.get();
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
    static bool valid_filesize(size_t);
#if SDL_DEBUG
    static bool check_page(page_head const *, pageIndex);
#endif
    void load_all();
    page_head const * load_page_nolock(pageIndex);
private:
    std::mutex m_mutex; // will improve
    PagePoolFile m_file;
    std::unique_ptr<char[]> m_alloc; // huge memory
    std::vector<bool> m_slot_commit;
    struct info_t {
        size_t filesize = 0;
        size_t page_count = 0;
        size_t slot_count = 0;
        size_t last_slot() const {
            return slot_count - 1;
        }
        size_t last_slot_page_count() const {
            static_assert(is_power_two(slot_page_num), "");
            const size_t n = page_count % slot_page_num;
            return n ? n : slot_page_num;
        }
        size_t last_slot_size() const {
            return page_size * last_slot_page_count();
        }
    };
    info_t m; // read-only
};

} // pp
} // db
} // sdl

#endif // SDL_TEST_PAGE_POOL
#endif // __SDL_SYSTEM_PAGE_POOL_H__
