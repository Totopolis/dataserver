// sparse_set.h
//
#pragma once
#ifndef __SDL_SPATIAL_SPARSE_SET_HPP__
#define __SDL_SPATIAL_SPARSE_SET_HPP__

#include "dataserver/spatial/interval_set.h"
#include "dataserver/system/page_iterator.h"
#include <map>

namespace sdl { namespace db {

template<typename T>
class sparse_set : noncopyable { // to be tested with signed values
    using umask_t = uint64;
    static constexpr umask_t seg_size = sizeof(umask_t) * 8;
    static constexpr umask_t seg_mask = seg_size - 1;
    static_assert(seg_size == 64, "");
	static_assert(sizeof(T) <= sizeof(umask_t), "");
    using map_type = std::map<size_t, umask_t>; //FIXME: std::map<int, uint64>
	std::unique_ptr<map_type> m_map;
    size_t m_size = 0;
public:
    using value_type = T;
    sparse_set(): m_map(new map_type) {}
    sparse_set(sparse_set && src) noexcept
        : m_map(std::move(src.m_map))
        , m_size(src.m_size) {
        src.m_size = 0;
		static_check_is_nothrow_move_assignable(m_map);
		SDL_ASSERT(m_map);
		SDL_ASSERT(!src.m_map);
	}
    size_t size() const {
        return m_size;
    }
    size_t contains() const {
        return m_map->size();
    }
    void swap(sparse_set & src) noexcept {
        m_map.swap(src.m_map);
        std::swap(m_size, src.m_size);
    }
    sparse_set & operator=(sparse_set && v) noexcept {
        (*this).swap(v);
        return *this;
    }
    bool empty() const {
        SDL_ASSERT((m_size > 0) == (m_map && !m_map->empty()));
        return 0 == m_size;
    }
    void clear() {
        m_map->clear();
        m_size = 0;
    }
    bool insert(value_type);
    
    template<class fun_type>
    break_or_continue for_each(fun_type && fun) const;

    std::vector<value_type> copy_to_vector() const;
private:
    static value_type make_value(const size_t seg, const size_t bit) {
        const umask_t base = (umask_t)seg * seg_size;
        return (value_type)(base + bit);
    }
};

template<typename value_type> inline
bool sparse_set<value_type>::insert(value_type const value) {
    const size_t seg = (umask_t)(value) / seg_size;
    const size_t bit = (umask_t)(value) & seg_mask;
    const umask_t test = umask_t(1) << bit;
    SDL_ASSERT(test < (umask_t)(-1));
    SDL_ASSERT(bit == ((umask_t)value % seg_size));
    SDL_ASSERT(value == make_value(seg, bit));
    umask_t & slot = (*m_map)[seg];
    if (slot & test) {
        return false;
    }
    slot |= test;
    ++m_size;
    return true;
}

template<typename value_type>
template<class fun_type> break_or_continue
sparse_set<value_type>::for_each(fun_type && fun) const {
    auto const last = m_map->end();
    for(auto it = m_map->begin(); it != last; ++it) {
        const umask_t base = it->first * seg_size;
        umask_t slot = it->second;
        SDL_ASSERT(slot);
        for (size_t bit = 0; slot; ++bit) {
            SDL_ASSERT(bit < seg_size);
            if (slot & 1) {
                const umask_t uvalue = base + bit;
                const value_type value = (value_type)(uvalue);
                SDL_ASSERT(value == make_value(it->first, bit));
                if (is_break(fun(value))) {
                    return bc::break_;
                }
            }
            slot >>= 1;
        }
    }
    return bc::continue_;
}

template<typename value_type>
std::vector<value_type>
sparse_set<value_type>::copy_to_vector() const {
    A_STATIC_ASSERT_IS_POD(value_type);
    std::vector<value_type> result(size());
    auto it = result.begin();
    for_each([&it](value_type value){
        *it++ = value;
        return bc::continue_;
    });
    return result;
}


template<typename T>
struct sparse_set_trait {
    using type = interval_set<T>;
};

//FIXME: any integral
template<> struct sparse_set_trait<int64>  { using type = sparse_set<int64>; };
template<> struct sparse_set_trait<uint64> { using type = sparse_set<uint64>; };

template<typename T>
using sparse_set_t = typename sparse_set_trait<T>::type;

} // db
} // sdl

#endif // __SDL_SPATIAL_SPARSE_SET_HPP__