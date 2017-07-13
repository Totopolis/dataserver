// page_bpool.h
//
#pragma once
#ifndef __SDL_BPOOL_PAGE_BPOOL_H__
#define __SDL_BPOOL_PAGE_BPOOL_H__

#include "dataserver/bpool/block_head.h"
#include "dataserver/bpool/file.h"
#include "dataserver/common/spinlock.h"

namespace sdl { namespace db { namespace bpool {

struct pool_info_t : public pool_limits {
    size_t const filesize = 0;
    size_t const page_count = 0;
    size_t const block_count = 0;
    size_t last_block = 0;
    size_t last_block_page_count = 0;
    size_t last_block_size = 0;
    explicit pool_info_t(size_t);
    size_t get_block_size(const size_t b) const {
        SDL_ASSERT(b < block_count);
        return (b == last_block) ? last_block_size : block_size;
    }
};

class base_page_bpool {
protected:
    explicit base_page_bpool(const std::string & fname);
    ~base_page_bpool();
public:
    static bool valid_filesize(size_t);
protected:
    PagePoolFile m_file;
};

class page_bpool final : base_page_bpool {
    sdl_noncopyable(page_bpool)
public:
    page_bpool(const std::string & fname, size_t, size_t);
    explicit page_bpool(const std::string & fname);
    ~page_bpool();
public:
    bool is_open() const;
    void const * start_address() const;
    size_t page_count() const;
    page_head const * lock_page(pageIndex); // load_page
    bool unlock_page(pageIndex);
#if SDL_DEBUG
    bool assert_page(pageFileID);
#endif
private:
    using page32 = pageFileID::page32;
    const size_t min_pool_size;
    const size_t max_pool_size;
    const pool_info_t info;
    std::mutex m_mutex;
    mutable atomic_flag_init m_flag;
    std::vector<block_index> m_block;
};

}}} // sdl

#endif // __SDL_BPOOL_PAGE_BPOOL_H__
