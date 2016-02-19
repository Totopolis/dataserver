// index_page.h
//
#ifndef __SDL_SYSTEM_INDEX_PAGE_H__
#define __SDL_SYSTEM_INDEX_PAGE_H__

#pragma once

#include "page_head.h"

namespace sdl { namespace db {

#pragma pack(push, 1) 

struct index_page_row_meta;
struct index_page_row_info;

struct index_page_row
{
    using meta = index_page_row_meta;
    using info = index_page_row_info;

    struct data_type {
        bitmask8    statusA;    // 1 byte
        uint32      key;        //FIXME: depends on key type and size
        pageFileID  page;       // 6 bytes
    };
    union {
        data_type data;
        char raw[sizeof(data_type)];
    };
};

#pragma pack(pop)

struct index_page_row_meta: is_static {

    typedef_col_type_n(index_page_row, statusA);
    typedef_col_type_n(index_page_row, key);
    typedef_col_type_n(index_page_row, page);

    typedef TL::Seq<
        statusA
        ,key
        ,page
    >::Type type_list;
};

struct index_page_row_info: is_static {
    static std::string type_meta(index_page_row const &);
    static std::string type_raw(index_page_row const &);
};

} // db
} // sdl

#endif // __SDL_SYSTEM_INDEX_PAGE_H__