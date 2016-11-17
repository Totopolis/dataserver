// sparse_set.h
//
#pragma once
#ifndef __SDL_SPATIAL_SPARSE_SET_HPP__
#define __SDL_SPATIAL_SPARSE_SET_HPP__

#include "dataserver/spatial/interval_set.h"
#include <unordered_map>

namespace sdl { namespace db { namespace sparse {

template<typename pk0_type>
class sparse_set : noncopyable {
    using umask_t = uint64;
    static constexpr umask_t seg_size = sizeof(umask_t) * 8;
    static constexpr umask_t seg_mask = seg_size - 1;
    static_assert(sizeof(pk0_type) == sizeof(umask_t), "");
    static_assert(seg_size == 64, "");
    using map_type = std::unordered_map<size_t, umask_t>;
    map_type m_mask;
    size_t m_size = 0;
public:
    sparse_set() {
        static_assert(sizeof(pk0_type) <= sizeof(umask_t), "");
    }
    sparse_set(sparse_set && src) //noexcept
        : m_mask(std::move(src.m_mask))
        , m_size(src.m_size) {
		//static_check_is_nothrow_move_assignable(m_set);
	}
    size_t size() const {
        return m_size;
    }
    size_t contains() const {
        return m_mask.size();
    }
    void swap(sparse_set & src) noexcept {
        m_mask.swap(src.m_mask);
        std::swap(m_size, src.m_size);
    }
    sparse_set & operator=(sparse_set && v) noexcept {
        (*this).swap(v);
        return *this;
    }
    bool empty() const {
        return m_mask.empty();
    }
    void clear() {
        m_mask.clear();
        m_size = 0;
    }
    bool insert(pk0_type const value) {
        const size_t seg = (umask_t)(value) / seg_size;
        const size_t bit = (umask_t)(value) & seg_mask;
        const umask_t test = umask_t(1) << bit;
        SDL_ASSERT(test < (umask_t)(-1));
        SDL_ASSERT(bit == ((umask_t)value % seg_size));
        SDL_ASSERT(value == make_pk0(seg, bit));
        umask_t & slot = m_mask[seg];
        if (slot & test) {
            return false;
        }
        slot |= test;
        ++m_size;
        return true;
    }
private:
    static pk0_type make_pk0(const size_t seg, const size_t bit) {
        const umask_t base = (umask_t)seg * seg_size;
        const umask_t uvalue = base + bit;
        const pk0_type value = (pk0_type)(uvalue);
        return value;
    }
    template<class fun_type>
    break_or_continue for_each(fun_type && fun) const;
};

template<typename pk0_type>
template<class fun_type> break_or_continue
sparse_set<pk0_type>::for_each(fun_type && fun) const {
    auto const last = m_mask.end();
    for(auto it = m_mask.begin(); it != last; ++it) {
        const umask_t base = it->first * seg_size;
        umask_t slot = it->second;
        SDL_ASSERT(slot);
        for (size_t bit = 0; slot; ++bit) {
            SDL_ASSERT(bit < seg_size);
            if (slot & 1) {
                const umask_t uvalue = base + bit;
                const pk0_type value = (pk0_type)(uvalue);
                SDL_ASSERT(value == make_pk0(it->first, bit));
                if (is_break(fun(value))) {
                    return bc::break_;
                }
            }
            slot >>= 1;
        }
    }
    return bc::continue_;
}

template<typename T>
struct pk0_type_set {
    using type = interval_set<T>;
};
template<> struct pk0_type_set<int64> {
    using type = sparse_set<int64>;
};
template<> struct pk0_type_set<uint64> {
    using type = sparse_set<uint64>;
};

} // sparse
} // db
} // sdl

#endif // __SDL_SPATIAL_SPARSE_SET_HPP__