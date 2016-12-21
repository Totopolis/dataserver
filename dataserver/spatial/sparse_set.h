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
        return m_map->size();
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
        SDL_ASSERT((m_size > 0) == (m_map && !m_map->empty()));
        return 0 == m_size;
    }
    void clear() {
        m_map->clear();
        m_size = 0;
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
private:
    friend iterator;
    bool assert_bit_state(bit_state const & state) const;
    value_type dereference(bit_state const & it) const {
        SDL_ASSERT(assert_bit_state(it));
        return make_value(it.first.first->first, it.second);
    }
    void load_next(bit_state & it) const;
};

template<typename value_type>
bool sparse_set<value_type>::assert_bit_state(bit_state const & state) const {
    A_STATIC_CHECK_TYPE(const map_key, state.first.first->first); // segment (map key)
    A_STATIC_CHECK_TYPE(uint64, state.first.second->second); // mask (map value)
    A_STATIC_CHECK_TYPE(int, state.second); // bit offset in mask
    SDL_ASSERT(state.first.first != m_map->end());
    SDL_ASSERT(state.first.second == m_map->end());
    SDL_ASSERT((state.second >= 0) && (state.second < seg_size));
    SDL_ASSERT(state.first.first->second);
    SDL_ASSERT(state.first.first->second & (uint64(1) << state.second));
    SDL_ASSERT((state.first.first->second >> state.second) & 1);
    return true;
}

template<typename value_type>
bool sparse_set<value_type>::insert(const value_type value, bool_constant<false>) {
    const map_key seg = static_cast<map_key>(value / seg_size);
    const int bit = static_cast<map_key>(value & seg_mask);
    SDL_ASSERT(bit == static_cast<map_key>(value % seg_size));
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

template<typename value_type>
bool sparse_set<value_type>::insert(const value_type value, bool_constant<true>) {
    map_key seg;
    int bit;
    if (value < 0) {
        const value_type pos = - value - 1;
        SDL_ASSERT(pos >= 0);
        seg = - static_cast<map_key>(pos / seg_size) - 1;
        bit = seg_mask - static_cast<map_key>(pos & seg_mask);
        SDL_ASSERT(seg < 0);
        SDL_ASSERT(pos >= 0);
    }
    else {
        seg = static_cast<map_key>(value / seg_size);
        bit = static_cast<map_key>(value & seg_mask);
        SDL_ASSERT(bit == static_cast<map_key>(value % seg_size));
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
void sparse_set<value_type>::load_next(bit_state & state) const
{
    SDL_ASSERT(assert_bit_state(state));
    SDL_ASSERT(state.first.second == m_map->end());
    auto & first = state.first.first;
    int bit = ++(state.second);
    if (bit < seg_size) {
        uint64 slot = first->second;
        SDL_ASSERT(slot && (bit > 0));
        SDL_ASSERT((slot >> (bit - 1)) & 1);
        slot >>= bit;
        for (; slot; ++bit) {
            SDL_ASSERT(bit < seg_size);
            if (slot & 1) {
                state.second = bit;
                return;
            }
            slot >>= 1;
        }
        SDL_ASSERT(first != m_map->end());
    }
    ++first;
    state.second = 0;
    if (first != state.first.second) {
        uint64 slot = first->second;
        SDL_ASSERT(slot);
        for (; slot; ++(state.second)) {
            SDL_ASSERT(state.second < seg_size);
            if (slot & 1) {
                return;
            }
            slot >>= 1;
        }
        SDL_ASSERT(0);
    }
}

template<typename value_type>
typename sparse_set<value_type>::iterator
sparse_set<value_type>::begin() const
{
    auto const last = m_map->end();
    for(auto it = m_map->begin(); it != last; ++it) {
        A_STATIC_CHECK_TYPE(uint64, it->second);
        uint64 slot = it->second;
        SDL_ASSERT(slot);
        for (int bit = 0; slot; ++bit) {
            SDL_ASSERT(bit < seg_size);
            if (slot & 1) {
                return iterator(this, bit_state({it, last}, bit));
            }
            slot >>= 1;
        }
    }
    return this->end();
}

template<typename value_type>
typename sparse_set<value_type>::iterator
sparse_set<value_type>::end() const
{
    auto const last = m_map->end();
    return iterator(this, bit_state({last, last}, 0));
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
