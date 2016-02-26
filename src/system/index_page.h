// index_page.h
//
#ifndef __SDL_SYSTEM_INDEX_PAGE_H__
#define __SDL_SYSTEM_INDEX_PAGE_H__

#pragma once

#include "page_head.h"

namespace sdl { namespace db {

#pragma pack(push, 1) 

template<class T>
struct pair_key_t
{
    T k1;
    T k2;
};

template<class T>
struct index_page_row_t // > 7 bytes
{
    using key_type = T;

    struct data_type {
        bitmask8    statusA;    // 1 byte
        key_type    key;        // primary key
        pageFileID  page;       // 6 bytes
    };
    union {
        data_type data;
        char raw[sizeof(data_type)];
    };
};

enum { index_row_head_size = sizeof(bitmask8) + sizeof(pageFileID) };

#pragma pack(pop)

namespace impl {

template<scalartype::type v, typename T>
struct index_key_t {
    static const scalartype::type value = v;
    using type = T;
};

} // impl

template<scalartype::type v> using index_key_t = impl::index_key_t<v, typename scalartype_t<v>::type>;
template<scalartype::type v> using index_key = typename index_key_t<v>::type;

template<class T> inline
std::ostream & operator <<(std::ostream & out, pair_key_t<T> const & i) {
    out << "(" << i.k1 << ", " << i.k2 << ")";
    return out;
}

template<class fun_type>
void case_index_key(scalartype::type const v, fun_type fun)
{
    switch (v) {
    case scalartype::t_int:
        fun(index_key_t<scalartype::t_int>());
        break;
    case scalartype::t_bigint:
        fun(index_key_t<scalartype::t_bigint>());
        break;
    case scalartype::t_uniqueidentifier:
        fun(index_key_t<scalartype::t_uniqueidentifier>());
        break;
    default:
        SDL_ASSERT(0);
        break;
    }
}

} // db
} // sdl

#endif // __SDL_SYSTEM_INDEX_PAGE_H__