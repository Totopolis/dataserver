// page_bpool.h
//
#pragma once
#ifndef __SDL_BPOOL_PAGE_BPOOL_H__
#define __SDL_BPOOL_PAGE_BPOOL_H__

#include "dataserver/bpool/block_head.h"
#include "dataserver/bpool/file.h"
#include "dataserver/common/spinlock.h"
#include "dataserver/common/algorithm.h"

namespace sdl { namespace db { namespace bpool {

struct pool_info_t : public pool_limits {
    using T = pool_limits;
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

struct bit_info_t {
    size_t const bit_size = 0;
    size_t byte_size = 0;
    size_t last_byte = 0;
    size_t last_byte_bits = 0;
    explicit bit_info_t(pool_info_t const &);
}; 

class thread_id_t {
public:
    using id_type = std::thread::id;
    using data_type = std::vector<id_type>;
    using size_bool = std::pair<size_t, bool>;
    static id_type get_id() {
        return std::this_thread::get_id();
    }
    size_t size() const {
        return m_data.size();
    }
    const data_type & data() const { 
        return m_data;
    }
    size_bool insert() {
        return insert(get_id());
    }
    size_bool insert(id_type);
    size_t find(id_type) const;
    bool erase(id_type);
private:
    data_type m_data; // must be sorted
};

class thread_tlb_t {
    using mask_type = std::vector<uint8>; // 262144 byte per 1 TB space; 1280 byte per 5 GB space
    using unique_mask = std::unique_ptr<mask_type>;
    using vector_mask = std::vector<unique_mask>;
public:
    explicit thread_tlb_t(pool_info_t const & in): pi(in), bi(in) {}
private:
    pool_info_t const & pi;
    bit_info_t const bi;   
    thread_id_t thread_id;
    vector_mask thread_msk;
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
    using page32 = pageFileID::page32;
public:
    page_bpool(const std::string & fname, size_t, size_t);
    explicit page_bpool(const std::string & fname): page_bpool(fname, 0, 0){}
    ~page_bpool();
public:
    bool is_open() const;
    void const * start_address() const;
    size_t page_count() const;
    page_head const * lock_page(pageIndex); // load_page
    bool unlock_page(pageIndex);
#if SDL_DEBUG
    bool assert_page(pageIndex);
#endif
private:
    /*struct async_data {
        std::mutex mutex;
        atomic_flag_init flag;
    };*/
private:
    const size_t min_pool_size;
    const size_t max_pool_size;
    const pool_info_t info;
    //mutable async_data async;
    std::vector<block_index> m_block; // block indexx
    //joinable_thread
    //thread_id_table
};

}}} // sdl

#include "dataserver/bpool/page_bpool.inl"

#endif // __SDL_BPOOL_PAGE_BPOOL_H__
