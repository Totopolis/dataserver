// sparse_set.h
//
#pragma once
#ifndef __SDL_SPATIAL_SPARSE_SET_H__
#define __SDL_SPATIAL_SPARSE_SET_H__

#include "dataserver/spatial/interval_set.h"
#include "dataserver/system/page_iterator.h"
#include <map>

namespace sdl { namespace db {

template<typename T>
class sparse_set : noncopyable {
    static constexpr int seg_size = 64; // divider
    static constexpr int seg_mask = seg_size - 1;
    static_assert(std::is_integral<T>::value, "");
    static_assert(sizeof(T) <= sizeof(uint64), "");
    enum { is_signed_value = std::is_signed<T>::value };
    using is_signed_constant = bool_constant<is_signed_value>;
    using map_key = int;
    using map_type = std::map<map_key, uint64>;
    using map_iterator = typename map_type::const_iterator;
    using bit_state = std::pair<std::pair<map_iterator, map_iterator>, int>; // pair<pair<begin, end>, bit>
private:
    std::unique_ptr<map_type> m_map;
    size_t m_size = 0;
    const map_type & cmap() const { return *m_map; }
    map_type & map() const { return *m_map; }
public:
    using iterator = forward_iterator<sparse_set const, bit_state>;
    using const_iterator = iterator;
    using value_type = T;
    sparse_set(): m_map(new map_type) {}
    sparse_set(sparse_set && src) noexcept
        : m_map(std::move(src.m_map))
        , m_size(src.m_size) {
        static_check_is_nothrow_move_assignable(m_map);
        src.m_size = 0;
        SDL_ASSERT(m_map);
        SDL_ASSERT(!src.m_map);
    }
    size_t size() const {
        return m_size;
    }
    size_t contains() const {
        return cmap().size();
    }
    void swap(sparse_set & src) noexcept {
        m_map.swap(src.m_map);
        std::swap(m_size, src.m_size);
    }
    sparse_set & operator=(sparse_set && v) noexcept {
        this->swap(v);
        return *this;
    }
    bool empty() const {
        SDL_ASSERT((m_size > 0) == (m_map && !cmap().empty()));
        return 0 == m_size;
    }
    void clear() {
        map().clear();
        m_size = 0;
    }
    bool find(value_type const v) const {
        static_assert((value_type(-1) < 0) == std::is_signed<T>::value, "");
        return find(v, is_signed_constant());
    }    
    bool insert(value_type const v) {
        static_assert((value_type(-1) < 0) == std::is_signed<T>::value, "");
        return insert(v, is_signed_constant());
    }    
    template<class fun_type>
    break_or_continue for_each(fun_type && fun) const;
    std::vector<value_type> copy_to_vector() const;

    iterator begin() const;
    iterator end() const;

    const_iterator cbegin() const { return begin(); }
    const_iterator cend() const { return end(); }
private:
    static value_type make_value(const map_key seg, const int bit, bool_constant<false>) {
        static_assert(!std::is_signed<value_type>::value, "");
        SDL_ASSERT(seg >= 0);
        SDL_ASSERT((bit >= 0) && (bit < seg_size));
        return static_cast<value_type>(seg) * seg_size + bit;
    }
    static value_type make_value(const map_key seg, const int bit, bool_constant<true>) {
        static_assert(std::is_signed<value_type>::value, "");
        static_assert(std::is_signed<map_key>::value, "");
        SDL_ASSERT((bit >= 0) && (bit < seg_size));
        if (seg < 0) {
            const value_type v = static_cast<value_type>(-1-seg) * seg_size + (seg_mask - bit);
            SDL_ASSERT(v >= 0);
            return - v - 1;
        }
        return static_cast<value_type>(seg) * seg_size + bit;
    }
    static value_type make_value(const map_key seg, const int bit) {
        return make_value(seg, bit, is_signed_constant());
    }
    bool insert(value_type, bool_constant<false>);
    bool insert(value_type, bool_constant<true>);
    bool find(value_type, bool_constant<false>) const;
    bool find(value_type, bool_constant<true>) const;
private:
    friend iterator;
    bool assert_bit_state(bit_state const & state) const;
    value_type dereference(bit_state const & it) const {
        SDL_ASSERT(assert_bit_state(it));
        return make_value(it.first.first->first, it.second);
    }
    void load_next(bit_state & it) const;
};

} // db
} // sdl

#include "dataserver/spatial/sparse_set.hpp"

#endif // __SDL_SPATIAL_SPARSE_SET_H__
