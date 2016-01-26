// iam_page.h
//
#ifndef __SDL_SYSTEM_IAM_PAGE_H__
#define __SDL_SYSTEM_IAM_PAGE_H__

#pragma once

#include "page_head.h"

namespace sdl { namespace db {

#pragma pack(push, 1) 

// Every table/index has its own set of IAM pages, which are combined into separate linked lists called IAM chains.
// Each IAM chain covers its own allocation unit—IN_ROW_DATA, ROW_OVERFLOW_DATA, and LOB_DATA.

// Each IAM page in the chain covers a particular GAM interval and represents the bitmap where each bit indicates
// if a corresponding extent stores the data that belongs to a particular allocation unit for a particular object. In addition,
// the first IAM page for the object stores the actual page addresses for the first eight object pages, which are stored in
// mixed extents.

struct iam_page_row_meta;
struct iam_page_row_info;

struct iam_page_row
{
    using meta = iam_page_row_meta;
    using info = iam_page_row_info;

    enum { slot_size = 8 };

    struct data_type { // 94 bytes (mixed extent)

        row_head    head;               //0x00 : 4 bytes
        uint32      sequenceNumber;     //0x04 : 
        uint8       _0x08[10];          //0x08 :
        uint16      status;             //0x12 :
        uint8       _0x14[12];          //0x14 :
        int32       objectId;           //0x20 :
        int16       indexId;            //0x24 :
        uint8       page_count;         //0x26 :
        uint8       _0x27;              //0x27 :
        pageFileID  start_pg;           //0x28 : 6 bytes
        pageFileID  slot_pg[slot_size]; //0x2E : 48 bytes
    };
    union {
        data_type data;
        char raw[sizeof(data_type)];
    };

    //FIXME: uniform extent
};

// Page Free Space (PFS) pages are data pages with only one data record.
// This data record has only one fixed length column, no NULL bitmap, and no variable length columns.
// The single fiixed length column is an 8088 byte array,
// and it stores one byte per page to track a number of statistics for that page.

// The first PFS page is at *:1 in each database file, and it stores the statistics for pages 0-8087 in that database file.
// There will be one PFS page for just about every 64MB of file size (8088 bytes * 8KB per page). 
// A large database file will use a long chain of PFS pages, linked together using the LastPage and NextPage pointers in the 96 byte header.
struct pfs_page_row
{
    enum { body_size = 8088 };
    struct data_type {
        row_head    head;               //0x00 : 4 bytes
        pfs_byte    body[body_size];    //0x04 : 8088 byte array 
    };
    union {
        data_type data;
        char raw[sizeof(data_type)];
    };
    pfs_byte const * begin() const {
        return data.body;
    }
    pfs_byte const * end() const {
        return data.body + body_size;
    }
};

#pragma pack(pop)

struct iam_page_row_meta: is_static {

    typedef_col_type_n(iam_page_row, head);
    typedef_col_type_n(iam_page_row, sequenceNumber);
    typedef_col_type_n(iam_page_row, status);
    typedef_col_type_n(iam_page_row, objectId);
    typedef_col_type_n(iam_page_row, indexId);
    typedef_col_type_n(iam_page_row, page_count);
    typedef_col_type_n(iam_page_row, start_pg);
    typedef_col_type_n(iam_page_row, slot_pg);

    typedef TL::Seq<
        head
        ,sequenceNumber
        ,status
        ,objectId
        ,indexId
        ,page_count
        ,start_pg
        ,slot_pg
    >::Type type_list;
};

struct iam_page_row_info: is_static {
    static std::string type_meta(iam_page_row const &);
    static std::string type_raw(iam_page_row const &);
};

} // db
} // sdl

#endif // __SDL_SYSTEM_IAM_PAGE_H__