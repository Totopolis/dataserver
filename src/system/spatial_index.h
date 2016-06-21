// spatial_index.h
//
#pragma once
#ifndef __SDL_SYSTEM_SPATIAL_INDEX_H__
#define __SDL_SYSTEM_SPATIAL_INDEX_H__

#include "page_head.h"
#include "spatial_type.h"

namespace sdl { namespace db {

#pragma pack(push, 1) 

struct spatial_root_row_meta;
struct spatial_root_row_info;

struct spatial_root_row {

    using meta = spatial_root_row_meta;
    using info = spatial_root_row_info;

    using pk0_type = int64;  //FIXME: template type ?

    struct data_type { // Record Size = 20                      // 166ca5f9 2304b582 11000000 0000c115 0b000100
        bitmask8        statusA;        // 0x00 : 1 byte        // 16
        spatial_cell    cell_id;        // 0x01 : 5 bytes       // 6c a5 f9 23 04
        pk0_type        pk0;            // 0x06 : 8 bytes       // b5 82 11 00 00 00 00 00
        pageFileID      page;           // 0x0e : 6 bytes       // c1 15 0b 00 01 00
    };
    union {
        data_type data;
        char raw[sizeof(data_type)];
    };
    recordType get_type() const { // Bits 1-3 of byte 0 give the record type
        return static_cast<recordType>((data.statusA.byte & 0xE) >> 1);
    }
};

#if 0
struct spatial_node_row {

    // Record Type = INDEX_RECORD
    // Record Attributes = NULL_BITMAP
    // FileId = 1
    // PageId = 16708
    // Row = 1
    // Level = 2
    // ChildFileId = 1
    // ChildPageId = 469608
    // Cell_id (key) = 0x640C529B04
    // pk0 (key) = 1469437
    // KeyHashValue = NULL
    // Row Size = 23

    //16640c52 9b04fd6b 16000000 0000682a 07000100 020000
    
    //FIXME: pk0 : template type ?

    struct data_type {  // Record Size = 23 
        bitmask8        statusA;            // 0x00 : 1 byte = 0x16                 // 16
        spatial_cell    cell_id;            // 0x01 : 5 bytes = 0x640C529B04        // 640c52 9b04
        uint64          pk0;                // 0x06 : 8 bytes = 1469437 = 0x166BFD  // fd6b 16000000 0000
        pageFileID      page;               // 0x0e : 6 bytes = [1:469608]          // 682a 07000100
        uint8           _0x14[3];           // 0x14 : 3 bytes                       // 020000
    };
    union {
        data_type data;
        char raw[sizeof(data_type)];
    };
};
#endif

struct spatial_page_row_meta;
struct spatial_page_row_info;

/* cell_attr:
0 – cell at least touches the object (but not 1 or 2)
1 – guarantee that object partially covers cell
2 – object covers cell */
struct spatial_page_row {
    
    using meta = spatial_page_row_meta;
    using info = spatial_page_row_info;

    using pk0_type = int64;  //FIXME: template type ?

    //0000000000000000: 10001700 60985955 04009e1f 00000000 000100e6
    //0000000000000014: 10000004 0000

    struct data_type { // Record Size = 26
        uint8           _0x00[4];       // 0x00 : 4 bytes                       // 10001700
        spatial_cell    cell_id;        // 0x04 : 5 bytes = 0x6098595504        // 60985adf 04          // Column 1 Offset 0x4 Length 5 Length (physical) 5
        pk0_type        pk0;            // 0x09 : 8 bytes = 2072064 = 0x1F9E00  // 009e1f 00000000 00   // Column 4 Offset 0x9 Length 8 Length (physical) 8
        uint16          cell_attr;      // 0x11 : 2 bytes = 1                   // 0100                 // Column 2 Offset 0x11 Length 2 Length (physical) 2
        uint32          SRID;           // 0x13 : 4 bytes = 4326 = 0x10E6       // e6 100000            // Column 3 Offset 0x13 Length 4 Length(physical) 4
        uint8           _0x17[3];       // 0x17 : 3 bytes = 040000              // 04 0000
    };
    union {
        data_type data;
        char raw[sizeof(data_type)];
    };
};

#pragma pack(pop)

struct spatial_root_row_meta: is_static {

    typedef_col_type_n(spatial_root_row, statusA);
    typedef_col_type_n(spatial_root_row, cell_id);
    typedef_col_type_n(spatial_root_row, pk0);
    typedef_col_type_n(spatial_root_row, page);

    typedef TL::Seq<
        statusA
        ,cell_id
        ,pk0
        ,page
    >::Type type_list;
};

struct spatial_root_row_info: is_static {
    static std::string type_meta(spatial_root_row const &);
    static std::string type_raw(spatial_root_row const &);
};

//------------------------------------------------------------------------

struct spatial_page_row_meta: is_static {

    typedef_col_type_n(spatial_page_row, cell_id);
    typedef_col_type_n(spatial_page_row, pk0);
    typedef_col_type_n(spatial_page_row, cell_attr);
    typedef_col_type_n(spatial_page_row, SRID);

    typedef TL::Seq<
        cell_id
        ,pk0
        ,cell_attr
        ,SRID
    >::Type type_list;
};

struct spatial_page_row_info: is_static {
    static std::string type_meta(spatial_page_row const &);
    static std::string type_raw(spatial_page_row const &);
};

//------------------------------------------------------------------------

} // db
} // sdl

#endif // __SDL_SYSTEM_SPATIAL_INDEX_H__