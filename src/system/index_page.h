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

namespace impl {

template<scalartype::type v, typename T>
struct index_key_t {
    enum { value = v };
    using type = T;
};

} // impl

template<scalartype::type v> struct index_key_t;
template<> struct index_key_t<scalartype::t_int>                : impl::index_key_t<scalartype::t_int, int32>{};
template<> struct index_key_t<scalartype::t_bigint>             : impl::index_key_t<scalartype::t_bigint, int64>{};
template<> struct index_key_t<scalartype::t_uniqueidentifier>   : impl::index_key_t<scalartype::t_uniqueidentifier, guid_t>{};
template<scalartype::type v> using index_key = typename index_key_t<v>::type;

#if 0
typedef TL::Seq<
    index_key_t<scalartype::t_int>
    ,index_key_t<scalartype::t_bigint> 
    ,index_key_t<scalartype::t_uniqueidentifier>
>::Type index_key_list; // registered types
#endif

} // db
} // sdl

#endif // __SDL_SYSTEM_INDEX_PAGE_H__