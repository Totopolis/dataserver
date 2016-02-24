// index_page.h
//
#ifndef __SDL_SYSTEM_INDEX_PAGE_H__
#define __SDL_SYSTEM_INDEX_PAGE_H__

#pragma once

#include "page_head.h"

namespace sdl { namespace db {

#pragma pack(push, 1) 

template<class T>
struct index_page_row_t
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

template<scalartype::type> struct index_key;
template<> struct index_key<scalartype::t_int>      { using type = int32; };
template<> struct index_key<scalartype::t_bigint>   { using type = int64; };

} // db
} // sdl

#endif // __SDL_SYSTEM_INDEX_PAGE_H__