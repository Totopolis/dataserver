// page_bpool.h
//
#pragma once
#ifndef __SDL_BPOOL_PAGE_BPOOL_H__
#define __SDL_BPOOL_PAGE_BPOOL_H__

#include "dataserver/bpool/file.h"
#include "dataserver/bpool/alloc.h"
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

#if 0
struct bit_info_t {
    size_t const bit_size = 0;
    size_t byte_size = 0;
    size_t last_byte = 0;
    size_t last_byte_bits = 0;
    explicit bit_info_t(pool_info_t const &);
}; 
#endif

class thread_id_t : noncopyable {
    enum { max_thread = pool_limits::max_thread };
public:
    using id_type = std::thread::id;
    using data_type = std::vector<id_type>; //todo: array<id_type, max_thread>
    using size_bool = std::pair<size_t, bool>;
    thread_id_t() {
        m_data.reserve(max_thread);
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
    size_bool insert(id_type); // throw if too many threads
    size_bool find(id_type) const;
    size_bool find() const {
        return find(get_id());
    }
    bool erase(id_type);
private:
    data_type m_data; // sorted for binary search
};

namespace unit {
    struct threadIndex{};
}
typedef quantity<unit::threadIndex, size_t> threadIndex;

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
public:
    const pool_info_t info;
    const size_t min_pool_size;
    const size_t max_pool_size;
};

class page_bpool final : base_page_bpool {
    using block32 = block_index::block32;
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
    block_head * first_block_head(block32) const;
    page_head const * zero_block_page(pageIndex);
    page_head const * lock_block_init(block32, pageIndex, threadIndex); // block is loaded from file
    page_head const * lock_block_head(block32, pageIndex, threadIndex, int); // block was loaded before
    bool unlock_block_head(block_index &, block32, pageIndex, threadIndex);
    bool free_unused_blocks();
    uint32 lastAccessTime(block32) const;
    size_t free_target_size() const;
    uint32 pageAccessTime() const;
#if SDL_DEBUG
    enum { trace_enable = 0 };
    bool assert_block_list(const char *, block32) const;
    bool assert_lock_list() const;
    bool assert_unlock_list() const;
    bool find_block(block32, block32) const;
    bool find_lock_block(block32) const;
    bool find_unlock_block(block32) const;
#endif
private:
    enum { adaptive_block_list = 1 }; // 0|1
    using lock_guard = std::lock_guard<std::mutex>;
    mutable std::mutex m_mutex;
    mutable atomic_flag_init m_flag;
    mutable uint32 m_pageAccessTime = 0;
    std::vector<block_index> m_block;
    thread_id_t m_thread_id;
    page_bpool_alloc m_alloc;
    block32 m_lock_block_list = 0;      // used block list
    block32 m_unlock_block_list = 0;    // unused block list
    //joinable_thread...
};

}}} // sdl

#include "dataserver/bpool/page_bpool.inl"

#endif // __SDL_BPOOL_PAGE_BPOOL_H__
