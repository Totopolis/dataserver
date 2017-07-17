// page_bpool.h
//
#pragma once
#ifndef __SDL_BPOOL_PAGE_BPOOL_H__
#define __SDL_BPOOL_PAGE_BPOOL_H__

#include "dataserver/bpool/block_head.h"
#include "dataserver/bpool/file.h"
#include "dataserver/bpool/vm_alloc.h"
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
    enum { sortorder = 1 }; // to be tested
public:
    using id_type = std::thread::id;
    using data_type = std::vector<id_type>; //FIXME: array_t<id_type, pool_limits::max_thread>
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
    data_type m_data; // must be sorted
};

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
public:
    const pool_info_t info;
    const size_t min_pool_size;
    const size_t max_pool_size;
};

class page_bpool final : base_page_bpool {
    sdl_noncopyable(page_bpool)
public:
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
    char * base_address() const {
        return m_alloc.base_address();
    }
    static block_head * get_block_head(page_head *);
#if SDL_DEBUG
    static bool check_page(page_head const *, pageIndex);
#endif
private:
    using lock_guard = std::lock_guard<std::mutex>;
    mutable std::mutex m_mutex;
    mutable atomic_flag_init m_flag;
    std::vector<block_index> m_block;
    thread_id_t m_thread_id;
    vm_alloc m_alloc;
    //joinable_thread
};

}}} // sdl

#include "dataserver/bpool/page_bpool.inl"

#endif // __SDL_BPOOL_PAGE_BPOOL_H__
