// index_page.h
//
#ifndef __SDL_SYSTEM_INDEX_PAGE_H__
#define __SDL_SYSTEM_INDEX_PAGE_H__

#pragma once

#include "page_head.h"
#include "page_info.h"

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

    std::string str() const {
        std::stringstream ss;
        ss << "statusA = " << to_string::type(data.statusA)
            << "\nkey = " << to_string::type(data.key)
            << "\npage = " << to_string::type(data.page);
        return ss.str();
    }
};

using index_page_row = index_page_row_t<uint64>; // uint64 = scalartype::t_bigint

#pragma pack(pop)

} // db
} // sdl

#endif // __SDL_SYSTEM_INDEX_PAGE_H__