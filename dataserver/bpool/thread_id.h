// thread_id.h
//
#pragma once
#ifndef __SDL_BPOOL_THREAD_ID_H__
#define __SDL_BPOOL_THREAD_ID_H__

#include "dataserver/bpool/block_head.h"
#include "dataserver/common/static_vector.h"
#include <thread>

namespace sdl { namespace db { namespace bpool {

class thread_id_t : noncopyable {
    enum { max_thread = pool_limits::max_thread };
public:
    using id_type = std::thread::id;
    using size_bool = std::pair<size_t, bool>;
    thread_id_t() {
        m_data.reserve(max_thread + 1);
    }
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
    void erase_pos(size_t const i) {
        m_data.erase(m_data.begin() + i);
    }
private:
    //using data_type = std::vector<id_type>;
    using data_type = static_vector<id_type, max_thread + 1>;
    data_type m_data; // sorted for binary search
};

namespace unit {
    struct threadIndex{};
}
typedef quantity<unit::threadIndex, size_t> threadIndex;

}}} // sdl

#endif // __SDL_BPOOL_THREAD_ID_H__
