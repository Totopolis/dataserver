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

using index_page_row = index_page_row_t<uint64>; // uint64 = scalartype::t_bigint

#pragma pack(pop)

struct index_page_row_info: is_static {
    static std::string type_meta(index_page_row const &);
    static std::string type_raw(index_page_row const &);
};

} // db
} // sdl

#endif // __SDL_SYSTEM_INDEX_PAGE_H__