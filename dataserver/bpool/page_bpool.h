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

class thread_id_t : noncopyable {
    enum { sortorder = 0 };
public:
    using id_type = std::thread::id;
    using data_type = std::vector<id_type>;
    using size_bool = std::pair<size_t, bool>;
    thread_id_t() {
        m_data.reserve(pool_limits::max_thread);
    }
    static id_type get_id() {
        return std::this_thread::get_id();
    }
    size_t size() const {
        return m_data.size();
    }
    size_bool insert() {
        return insert(get_id());
    }
    size_bool insert(id_type);
    size_t find(id_type) const;
    bool erase(id_type);
private:
    const data_type & data() const { 
        return m_data;
    }
private:
    data_type m_data; // must be sorted
};

class thread_tlb_t : noncopyable {
    using mask_type = std::vector<uint8>; // 262144 byte per 1 TB space; 1280 byte per 5 GB space
    using unique_mask = std::unique_ptr<mask_type>;
    using vector_mask = std::vector<unique_mask>;
public:
    explicit thread_tlb_t(pool_info_t const & in): bi(in) {}
    threadIndex insert();
private:
    bit_info_t const bi;   
    thread_id_t thread_id;
    vector_mask thread_mk;
};

class block_indexx : noncopyable {
    const pool_info_t & info;
    std::vector<block_index> m_data;
public:
    block_indexx(const pool_info_t & in): info(in) {
        m_data.resize(in.block_count);
        SDL_ASSERT(m_data.size() == info.last_block + 1);
    }
    block_index & find(pageIndex const i) {
        const size_t b = i.value() / pool_limits::block_page_num;
        if (info.last_block < b) {
            throw_error_t<block_index>("page not found");
        }
        return m_data[b];
    }
}; 

class page_bpool_file {
public:
    explicit page_bpool_file(const std::string & fname);
    static bool valid_filesize(size_t);
protected:
    PagePoolFile m_file;
};


class base_page_bpool : public page_bpool_file {
    using base = page_bpool_file;
public:
    const pool_info_t info;
    explicit base_page_bpool(const std::string & fname)
        : base(fname)
        , info(m_file.filesize())
    {}
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
    const size_t min_pool_size;
    const size_t max_pool_size;
    mutable std::mutex m_mutex;
    mutable atomic_flag_init m_flag;
    block_indexx m_block;
    thread_tlb_t m_tlb;
    //joinable_thread
};

}}} // sdl

#include "dataserver/bpool/page_bpool.inl"

#endif // __SDL_BPOOL_PAGE_BPOOL_H__
