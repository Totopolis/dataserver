// thread_id.h
//
#pragma once
#ifndef __SDL_BPOOL_THREAD_ID_H__
#define __SDL_BPOOL_THREAD_ID_H__

#include "dataserver/bpool/block_head.h"
#include "dataserver/common/array.h"
#include <atomic>
#include <thread>

namespace sdl { namespace db { namespace bpool {

class thread_mask_t : noncopyable {
    static constexpr size_t index_size = megabyte<512>::value; // 2^29, 536,870,912
    static constexpr size_t max_index = terabyte<1>::value / index_size; // 2048
    enum { block_size = pool_limits::block_size };
    enum { chunk_block_num = 64 }; // # of blocks in uint64 mask
    enum { chunk_size = block_size * chunk_block_num };
    enum { index_block_num = index_size / block_size }; // 8192 = 128 * 64
    enum { mask_div = 8 * sizeof(uint64) }; // 64 = 2^6
    enum { mask_hex = mask_div - 1 };
    enum { mask_size = index_block_num / mask_div }; // 128, 1 bit per block
    static_assert(mask_size * chunk_size == index_size, "");
    using mask_t = array_t<uint64, mask_size>;
    using mask_p = std::unique_ptr<mask_t>;
    using data_type = std::vector<mask_p>; //FIXME: custom allocator
public:
    explicit thread_mask_t(size_t filesize);
    bool is_block(size_t) const;
    void clr_block(size_t);
    void set_block(size_t);
    size_t size() const {
        return m_block_count;
    }
    bool operator[](size_t const i) const {
        SDL_ASSERT(i < size());
        return is_block(i);
    }
    data_type const & data() const {
        return m_index;
    }
    template<class fun_type>
    void for_each_block(fun_type &&) const;
    void shrink_to_fit();
    void clear();
private:
    bool empty(mask_t const &) const;
private:
    const size_t m_index_count;
    const size_t m_block_count;
    data_type m_index;
};

inline bool thread_mask_t::empty(mask_t const & m) const {
    for (const auto & v : m) {
        if (v)
            return false;
    }
    return true;
}

class thread_id_t : noncopyable {
    enum { max_thread = pool_limits::max_thread };
public:
    using thread_id = std::thread::id;
    typedef thread_mask_t * const mask_ptr;
    struct pos_mask {
        threadIndex const first;
        mask_ptr second;
        pos_mask(): first(max_thread), second(nullptr) {}
        pos_mask(threadIndex f, mask_ptr s): first(f), second(s) {}
    };
    explicit thread_id_t(size_t filesize);
    static thread_id get_id() {
        return std::this_thread::get_id();
    }
    static constexpr size_t max_size() {
        return max_thread;
    }
    size_t size() const {
        SDL_ASSERT(m_size <= max_size());
        return m_size;
    }
    pos_mask insert() {
        return insert(get_id());
    }
    pos_mask insert(thread_id); // throw if too many threads
    bool erase(thread_id);
    pos_mask find(thread_id);
    pos_mask find() {
        return find(get_id());
    }
private:
    static bool empty(thread_id id) {
        return id == thread_id();
    }
#if 0
    enum { use_hash = 0 }; // to be tested
    static size_t hash_id(id_type const & id) {
        std::hash<std::thread::id> hasher;
        return hasher(id) % max_thread;
    }
#endif
    using unique_mask = std::unique_ptr<thread_mask_t>;
    using id_mask = std::pair<thread_id, unique_mask>;
    using data_type = array_t<id_mask, max_thread>;
    const thread_id init_thread_id;
    const size_t m_filesize;
    data_type m_data;
    std::atomic<int> m_size;
};

}}} // sdl

#include "dataserver/bpool/thread_id.inl"

#endif // __SDL_BPOOL_THREAD_ID_H__
