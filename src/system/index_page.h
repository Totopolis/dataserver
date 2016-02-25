// index_page.h
//
#ifndef __SDL_SYSTEM_INDEX_PAGE_H__
#define __SDL_SYSTEM_INDEX_PAGE_H__

#pragma once

#include "page_head.h"

namespace sdl { namespace db {

#pragma pack(push, 1) 

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

#pragma pack(pop)

template<scalartype::type v, typename T>
struct index_key_t {
    enum { value = v };
    using type = T;
};

typedef TL::Seq<
    index_key_t<scalartype::t_int, int32>
    ,index_key_t<scalartype::t_bigint, int64>
    ,index_key_t<scalartype::t_uniqueidentifier, guid_t>
>::Type index_key_list; // registered types

namespace impl {

template<scalartype::type, class type_list> 
struct index_key_select;

template<scalartype::type v> 
struct index_key_select<v, NullType> {
    using type = void;
};

template <scalartype::type v, class T, class U>
struct index_key_select<v, Typelist<T, U>> {
    using type = typename Select<
        T::value == v, 
        typename T::type,
        typename index_key_select<v, U>::type>::Result;
};

} // impl

template<scalartype::type v> 
using index_key = typename impl::index_key_select<v, index_key_list>::type;

} // db
} // sdl

#endif // __SDL_SYSTEM_INDEX_PAGE_H__