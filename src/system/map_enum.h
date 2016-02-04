// map_enum.h
//
#ifndef __SDL_SYSTEM_MAP_ENUM_H__
#define __SDL_SYSTEM_MAP_ENUM_H__

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
    mapped_type & 
    get(key_type const & id, typename enum_1::type const t) {
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
    mapped_type & 
    get(key_type const & id,
        typename enum_1::type const t1,
        typename enum_2::type const t2) 
    {
        return table[static_cast<int>(t1)][static_cast<int>(t2)][id];
    }
};

} // db
} // sdl

#endif // __SDL_SYSTEM_DATABASE_H__