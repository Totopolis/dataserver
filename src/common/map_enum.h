// map_enum.h
//
#ifndef __SDL_COMMON_MAP_ENUM_H__
#define __SDL_COMMON_MAP_ENUM_H__

#pragma once

namespace sdl { namespace db {

template<class map_type, class enum_1>
class map_enum_1 : noncopyable
{
    map_type table[enum_1::size];
public:
    using key_type = typename map_type::key_type;
    using mapped_type = typename map_type::mapped_type;

    mapped_type const * 
    find(key_type const & id, typename enum_1::type const t) const {
        auto & m = table[static_cast<int>(t)];
        auto it = m.find(id);
        if (it != m.end()) {
            return &(it->second);
        }
        return nullptr;
    }
    mapped_type & operator()(key_type const & id, typename enum_1::type const t) {
        return table[static_cast<int>(t)][id];
    }
};

template<class map_type, class enum_1, class enum_2>
class map_enum_2 : noncopyable
{
    map_type table[enum_1::size][enum_2::size];
public:
    using key_type = typename map_type::key_type;
    using mapped_type = typename map_type::mapped_type;

    mapped_type const * 
    find(key_type const & id, 
        typename enum_1::type const t1,
        typename enum_2::type const t2) const
    {
        auto & m = table[static_cast<int>(t1)][static_cast<int>(t2)];
        auto it = m.find(id);
        if (it != m.end()) {
            return &(it->second);
        }
        return nullptr;
    }
    mapped_type & operator()(
        key_type const & id,
        typename enum_1::type const t1,
        typename enum_2::type const t2) 
    {
        return table[static_cast<int>(t1)][static_cast<int>(t2)][id];
    }
};

template<class Key, class T>
class compact_map {
public:
    using key_type = Key;
    using mapped_type = T;
    using value_type = std::pair<Key, T>;
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

} // db
} // sdl

#endif // __SDL_COMMON_MAP_ENUM_H__