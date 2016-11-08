// compact_map.h
//
#pragma once
#ifndef __SDL_COMMON_COMPACT_MAP_H__
#define __SDL_COMMON_COMPACT_MAP_H__

#include "dataserver/common/common.h"

namespace sdl {

template<class Key, class T>
class compact_map {
public:
    using key_type = Key;
    using mapped_type = T;
    using value_type = std::pair<key_type, mapped_type>;
private:
    using vector_type = std::vector<value_type>;
    vector_type data;
public:
    using const_iterator = typename vector_type::const_iterator;
    compact_map() = default;

    const_iterator begin() const {
        return data.begin();
    }
    const_iterator end() const {
        return data.end();
    }
    const_iterator find(const key_type & k) const {
        auto const i = std::lower_bound(data.begin(), data.end(), k, 
            [](value_type const & x, const key_type & k) {
            return x.first < k;
        });
        if ((i != data.end()) && !(k < i->first)) {
            return i;
        }
        return data.end();
    }
    mapped_type & operator[](const key_type & k) {
        auto const i = std::lower_bound(data.begin(), data.end(), k, 
            [](value_type const & x, const key_type & k) {
            return x.first < k;
        });
        if (i != data.end()) {
            if (!(k < i->first)) {
                return i->second;
            }
            return data.emplace(i, k, T{})->second;
        }
        else {
            data.emplace_back(k, T{});
            return data.back().second;
        }
    }
};

} // sdl

#endif // __SDL_COMMON_COMPACT_MAP_H__