// thread_id.h
//
#pragma once
#ifndef __SDL_BPOOL_THREAD_ID_H__
#define __SDL_BPOOL_THREAD_ID_H__

#include "dataserver/bpool/block_head.h"
#include "dataserver/common/static_vector.h"
#include <thread>

namespace sdl { namespace db { namespace bpool {

class thread_mask_t : noncopyable {
    enum { node_byte_size = gigabyte<8>::value / megabyte<1>::value / 8 };
    enum { node_elem_size = node_byte_size / sizeof(uint64) };
    static_assert(node_byte_size == 1024, "");
    static_assert(node_elem_size == 128, "");
    using node_t = static_vector<uint64, node_elem_size>;
    using node_ptr = std::unique_ptr<node_t>; //node_ptr, node_p
    using T = std::pair<node_t, node_ptr>; // node_link
public:
    thread_mask_t() = default;
    bool empty() const {
        return !m_head;
    }
    void clear() {
        m_head.reset();
    }
    void resize(size_t filesize); // size in bytes
private:
    node_ptr m_head;
};

class thread_id_t : noncopyable {
    enum { max_thread = pool_limits::max_thread };
public:
    using id_type = std::thread::id;
    using size_bool = std::pair<size_t, bool>;
    thread_id_t() = default;
    static id_type get_id() {
        return std::this_thread::get_id();
    }
    bool empty() const {
        return m_data.empty();
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
    bool erase_id(id_type);
    void erase_pos(size_t);
private:
    using data_type = static_vector<id_type, max_thread + 1>;
    data_type m_data; // sorted for binary search
};

namespace unit {
    struct threadIndex{};
}
typedef quantity<unit::threadIndex, size_t> threadIndex;

}}} // sdl

#endif // __SDL_BPOOL_THREAD_ID_H__
