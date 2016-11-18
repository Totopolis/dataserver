// sparse_set.h
//
#pragma once
#ifndef __SDL_SPATIAL_SPARSE_SET_HPP__
#define __SDL_SPATIAL_SPARSE_SET_HPP__

#include "dataserver/spatial/interval_set.h"
//#include "dataserver/system/page_iterator.h"
#include <map>

namespace sdl { namespace db {

template<typename T>
class sparse_set : noncopyable {
    static constexpr int seg_size = 64; // divider
    static constexpr int seg_mask = seg_size - 1;
    static_assert(std::is_integral<T>::value, "");
    static_assert(sizeof(T) <= sizeof(uint64), "");
    using is_signed_constant = bool_constant<std::is_signed<T>::value>;
    using map_type = std::map<int, uint64>;
	std::unique_ptr<map_type> m_map;
    size_t m_size = 0;
public:
    //FIXME: const_iterator
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
    bool insert(value_type v) {
        static_assert((value_type(-1) < 0) == std::is_signed<T>::value, "");
        return insert(v, is_signed_constant());
    }    
    template<class fun_type>
    break_or_continue for_each(fun_type && fun) const;
    std::vector<value_type> copy_to_vector() const;
private:
    static value_type make_value(const int seg, const int bit, bool_constant<false>) {
        SDL_ASSERT(seg >= 0);
        SDL_ASSERT((bit >= 0) && (bit < seg_size));
        return static_cast<value_type>(seg) * seg_size + bit;
    }
    static value_type make_value(const int seg, const int bit, bool_constant<true>) {
        SDL_ASSERT((bit >= 0) && (bit < seg_size));
        if (seg < 0) {
            const value_type v = static_cast<value_type>(-1-seg) * seg_size + (seg_mask - bit);
            SDL_ASSERT(v >= 0);
            return - v - 1;
        }
        return static_cast<value_type>(seg) * seg_size + bit;
    }
    static value_type make_value(const int seg, const int bit) {
        return make_value(seg, bit, is_signed_constant());
    }
    bool insert(const value_type value, bool_constant<false>) {
        const int seg = static_cast<int>(value / seg_size);
        const int bit = static_cast<int>(value & seg_mask);
        SDL_ASSERT(bit == static_cast<int>(value % seg_size));
        SDL_ASSERT((bit >= 0) && (bit < seg_size));
        const uint64 flag = uint64(1) << bit;
        SDL_ASSERT(flag < (uint64)(-1));
        SDL_ASSERT(value == make_value(seg, bit));
        uint64 & slot = (*m_map)[seg];
        if (slot & flag) {
            return false;
        }
        slot |= flag;
        ++m_size;
        return true;
    }
    bool insert(const value_type value, bool_constant<true>) {
        int seg, bit;
        if (value < 0) {
            const value_type pos = - value - 1;
            seg = - static_cast<int>(pos / seg_size) - 1;
            bit = seg_mask - static_cast<int>(pos & seg_mask);
            SDL_ASSERT(seg < 0);
            SDL_ASSERT(pos >= 0);
        }
        else {
            seg = static_cast<int>(value / seg_size);
            bit = static_cast<int>(value & seg_mask);
            SDL_ASSERT(bit == static_cast<int>(value % seg_size));
            SDL_ASSERT(seg >= 0);
        }
        SDL_ASSERT((bit >= 0) && (bit < seg_size));
        const uint64 flag = uint64(1) << bit;
        SDL_ASSERT(flag < (uint64)(-1));
        SDL_ASSERT(value == make_value(seg, bit));
        uint64 & slot = (*m_map)[seg];
        if (slot & flag) {
            return false;
        }
        slot |= flag;
        ++m_size;
        return true;
    }
};

template<typename value_type>
template<class fun_type> break_or_continue
sparse_set<value_type>::for_each(fun_type && fun) const {
    auto const last = m_map->end();
    for(auto it = m_map->begin(); it != last; ++it) {
        A_STATIC_CHECK_TYPE(uint64, it->second);
        uint64 slot = it->second;
        SDL_ASSERT(slot);
        for (int bit = 0; slot; ++bit) {
            SDL_ASSERT(bit < seg_size);
            if (slot & 1) {
                if (is_break(fun(make_value(it->first, bit)))) {
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
    SDL_ASSERT(it == result.end());
    return result;
}

template<typename T>
struct sparse_set_trait {
    using type = interval_set<T>;
};

template<> struct sparse_set_trait<int16>  { using type = sparse_set<int16>; };
template<> struct sparse_set_trait<int32>  { using type = sparse_set<int32>; };
template<> struct sparse_set_trait<int64>  { using type = sparse_set<int64>; };

template<> struct sparse_set_trait<uint16> { using type = sparse_set<uint16>; };
template<> struct sparse_set_trait<uint32> { using type = sparse_set<uint32>; };
template<> struct sparse_set_trait<uint64> { using type = sparse_set<uint64>; };

template<typename T>
using sparse_set_t = typename sparse_set_trait<T>::type;

} // db
} // sdl

#endif // __SDL_SPATIAL_SPARSE_SET_HPP__