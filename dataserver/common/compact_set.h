// compact_set.h
//
#pragma once
#ifndef __SDL_COMMON_COMPACT_SET_H__
#define __SDL_COMMON_COMPACT_SET_H__

#include "dataserver/common/common.h"

namespace sdl {

template<class Key, class Compare>
struct equal_to {
    bool operator()(const Key & lhs, const Key & rhs) const {
        return !(Compare()(lhs, rhs) || Compare()(rhs, lhs));
    }
};

template<class Key, class Compare = std::less<Key>, class Equal = void>
class compact_set {
    using vector_type = std::vector<Key>;
    vector_type data;
public:
    using key_type = Key;
    using iterator = typename vector_type::iterator;
    using const_iterator = typename vector_type::const_iterator;
private:
    template<class T>
    static bool is_same(key_type const & x, key_type const & y, identity<T>) {
        return T()(x, y);
    }
    static bool is_same(key_type const & x, key_type const & y, identity<void>) {
        return x == y;
    }
    static bool is_same(key_type const & x, key_type const & y) {
        return is_same(x, y, identity<Equal>());
    }
public:
    compact_set() = default;
    compact_set(compact_set && src): data(std::move(src.data)) {}
    void swap(compact_set & src) {
        data.swap(src.data);
    }
    iterator lower_bound(key_type const & value) {
        return std::lower_bound(data.begin(), data.end(), value, Compare());
    }
    iterator find(key_type const & value) {
        iterator const pos = std::lower_bound(data.begin(), data.end(), value, Compare());
        if (pos != data.end()) {
            if (is_same(*pos, value)) {
                return pos;
            }
            return data.end();
        }
        return pos;
    }
    iterator erase(iterator it) {
        SDL_ASSERT(it != data.end());
        size_t const pos = std::distance(data.begin(), it);
        data.erase(it);
        return data.begin() + pos;
    }
    void erase(iterator first, iterator last) {
        SDL_ASSERT(!(last < first));
        data.erase(first, last);
    }
    bool empty() const {
        return data.empty();
    }
    void clear() {
        data.clear();
    }
    size_t size() const {
        return data.size();
    }
    iterator begin() {
        return data.begin();
    }
    iterator end() {
        return data.end();
    }
    const_iterator cbegin() const {
        return data.cbegin();
    }
    const_iterator cend() const {
        return data.cend();
    }
   bool insert(key_type const & value) {
        iterator const pos = lower_bound(value);
        if (pos != data.end()) {
            if (is_same(*pos, value)) {
                return false;
            }
            data.insert(pos, value);
            return true;
        }
        data.push_back(value);
        return true;
    }
    bool insert(iterator const pos, key_type const & value) {
        if (pos == data.end()) {
            SDL_ASSERT(empty() || Compare()(data.back(), value));
            data.push_back(value);
        }
        else {
            SDL_ASSERT(Compare()(value, *pos));
            data.insert(pos, value);
        }
        return true;
    }
};

} // sdl

#endif // __SDL_COMMON_COMPACT_SET_H__