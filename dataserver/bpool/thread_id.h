// thread_id.h
//
#pragma once
#ifndef __SDL_BPOOL_THREAD_ID_H__
#define __SDL_BPOOL_THREAD_ID_H__

#include "dataserver/bpool/block_head.h"
#include "dataserver/common/array.h"
#include <thread>

namespace sdl { namespace db { namespace bpool { namespace utils {

template<size_t div>
inline constexpr size_t round_up_div(size_t const s) {
    return (s + div - 1) / div;
}

} // utils

class thread_mask_t : noncopyable {
    static constexpr size_t index_size = megabyte<512>::value; // 2^29, 536,870,912
    static constexpr size_t max_index = terabyte<1>::value / index_size; // 2048
    enum { block_size = pool_limits::block_size };
    enum { index_block_num = index_size / block_size }; // 8192 = 128 * 64
    enum { mask_div = 8 * sizeof(uint64) }; // 64
    enum { mask_hex = mask_div - 1 };
    enum { mask_size = index_block_num / mask_div }; // 128, 1 bit per block
    using mask_t = array_t<uint64, mask_size>;
    using mask_p = std::unique_ptr<mask_t>;
    using vector_mask = std::vector<mask_p>;
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
    vector_mask const & data() const {
        return m_index;
    }
    void shrink_to_fit();
private:
    bool empty(mask_t const &) const;
private:
    const size_t m_index_count;
    const size_t m_block_count;
    vector_mask m_index;
};

class thread_id_t : noncopyable {
    enum { max_thread = pool_limits::max_thread };
    using size_bool = std::pair<size_t, bool>;
public:
    using id_type = std::thread::id;
    explicit thread_id_t(size_t);
    static id_type get_id() {
        return std::this_thread::get_id();
    }
    size_t insert() {
        return insert(get_id());
    }
    size_t insert(id_type); // throw if too many threads
    bool erase(id_type);
    size_bool find(id_type) const;
    size_bool find() const {
        return find(get_id());
    }
private:
    enum { use_hash = 0 };
    static size_t hash_id(id_type const &);
    using unique_mask = std::unique_ptr<thread_mask_t>;
    using id_mask = std::pair<id_type, unique_mask>;
    using data_type = array_t<id_mask, max_thread>;
    const size_t m_filesize;
    data_type m_data;
};

namespace unit {
    struct threadIndex{};
}
typedef quantity<unit::threadIndex, size_t> threadIndex;

}}} // sdl

#endif // __SDL_BPOOL_THREAD_ID_H__
