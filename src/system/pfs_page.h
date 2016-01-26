// pfs_page.h
//
#ifndef __SDL_SYSTEM_PFS_PAGE_H__
#define __SDL_SYSTEM_PFS_PAGE_H__

#pragma once

#include "page_head.h"

namespace sdl { namespace db {

#pragma pack(push, 1) 

// Page Free Space (PFS) pages are data pages with only one data record.
// This data record has only one fixed length column, no NULL bitmap, and no variable length columns.
// The single fiixed length column is an 8088 byte array,
// and it stores one byte per page to track a number of statistics for that page.

// The first PFS page is at *:1 in each database file, and it stores the statistics for pages 0-8087 in that database file.
// There will be one PFS page for just about every 64MB of file size (8088 bytes * 8KB per page). 
// A large database file will use a long chain of PFS pages, linked together using the LastPage and NextPage pointers in the 96 byte header.
struct pfs_page_row
{
    enum { pfs_size = 8088 };
    struct data_type {
        row_head    head;               //0x00 : 4 bytes
        pfs_byte    body[pfs_size];    //0x04 : 8088 byte array 
    };
    union {
        data_type data;
        char raw[sizeof(data_type)];
    };
    pfs_byte const * begin() const {
        return data.body;
    }
    pfs_byte const * end() const {
        return data.body + pfs_size;
    }
    pfs_byte operator[](size_t i) const {
        SDL_ASSERT(i < pfs_size);
        return data.body[i];
    }
};

#pragma pack(pop)

} // db
} // sdl

#endif // __SDL_SYSTEM_PFS_PAGE_H__