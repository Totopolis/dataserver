// page_bpool.h
//
#pragma once
#ifndef __SDL_BPOOL_PAGE_BPOOL_H__
#define __SDL_BPOOL_PAGE_BPOOL_H__

#include "dataserver/bpool/file.h"
#include "dataserver/bpool/alloc.h"
#include "dataserver/bpool/thread_id.h"
#include "dataserver/bpool/block_list.h"
#include "dataserver/common/spinlock.h"
#include "dataserver/common/algorithm.h"

namespace sdl { namespace db { namespace bpool {

struct pool_info_t final {
    using T = pool_limits;
    size_t const filesize = 0;
    size_t const page_count = 0;
    size_t const block_count = 0;
    size_t last_block = 0;
    size_t last_block_page_count = 0;
    size_t last_block_size = 0;
    explicit pool_info_t(size_t);
    size_t block_size_in_bytes(const size_t b) const {
        SDL_ASSERT(b < block_count);
        return (b == last_block) ? last_block_size : T::block_size;
    }
    size_t block_page_count(const size_t b) const {
        SDL_ASSERT(b < block_count);
        return (b == last_block) ? last_block_page_count : T::block_page_num;
    }
};

//----------------------------------------------------------

class page_bpool_file {
protected:
    explicit page_bpool_file(const std::string & fname);
    ~page_bpool_file(){}
public:
    static bool valid_filesize(size_t);
    size_t filesize() const { 
        return m_file.filesize();
    }
protected:
    PagePoolFile m_file;
};

class base_page_bpool : public page_bpool_file {
protected:
    base_page_bpool(const std::string & fname, size_t, size_t);
    ~base_page_bpool(){}
    size_t free_pool_size() const {
        return m_free_pool_size;
    }
public:
    const pool_info_t info;
    const size_t min_pool_size;
    const size_t max_pool_size;
    size_t m_free_pool_size = 0;
};

//----------------------------------------------------------

class page_bpool final : base_page_bpool {
    using block32 = block_index::block32;
    using freelist = block_list_t::freelist;
    SDL_DEBUG_HPP(using tracef = block_list_t::tracef;)
public:
    sdl_noncopyable(page_bpool)
    page_bpool(const std::string & fname, size_t, size_t);
    explicit page_bpool(const std::string & fname): page_bpool(fname, 0, 0){}
    ~page_bpool();
public:
    bool is_open() const;
    void const * start_address() const;
    size_t page_count() const;
    page_head const * lock_page(pageIndex);
    bool unlock_page(pageIndex);
#if SDL_DEBUG
    bool assert_page(pageIndex);
#endif
private:
#if SDL_DEBUG
    bool valid_checksum(char const * block_adr, pageIndex);
#endif
    void load_zero_block();
    void read_block_from_file(char * block_adr, size_t);
    static page_head * get_block_page(char * block_adr, size_t);
    static block_head * get_block_head(page_head *);
    static block_head * first_block_head(char * block_adr);
    static uint32 realBlock(pageIndex const pageId) {
        SDL_ASSERT(pageId.value() < pool_limits::max_page);
        return pageId.value() / pool_limits::block_page_num;
    }
    block_head * first_block_head(block32) const;
    block_head * first_block_head(block32, freelist) const;
    page_head const * zero_block_page(pageIndex);
    page_head const * lock_block_init(block32, pageIndex, threadIndex); // block is loaded from file
    page_head const * lock_block_head(block32, pageIndex, threadIndex, int); // block was loaded before
    bool unlock_block_head(block_index &, block32, pageIndex, threadIndex);
    size_t free_unlock_blocks(size_t memory); // returns number of free blocks
    uint32 lastAccessTime(block32) const;
    uint32 pageAccessTime() const;
    SDL_DEBUG_HPP(enum { trace_enable = false };)
private:
    friend block_list_t; // for first_block_head
    using lock_guard = std::lock_guard<std::mutex>;
    mutable std::mutex m_mutex;
    mutable atomic_flag_init m_flag;
    mutable uint32 m_pageAccessTime = 0;
    std::vector<block_index> m_block;
    thread_id_t m_thread_id;
    page_bpool_alloc m_alloc;
    block_list_t m_lock_block_list;
    block_list_t m_unlock_block_list;
    block_list_t m_free_block_list;
    //joinable_thread...
};

}}} // sdl

#include "dataserver/bpool/page_bpool.inl"

#endif // __SDL_BPOOL_PAGE_BPOOL_H__
