// thread_id.h
//
#pragma once
#ifndef __SDL_BPOOL_THREAD_ID_H__
#define __SDL_BPOOL_THREAD_ID_H__

#include "dataserver/bpool/block_head.h"
#include <thread>

namespace sdl { namespace db { namespace bpool {

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

}}} // sdl

#endif // __SDL_BPOOL_THREAD_ID_H__
