// spatial_index.h
//
#pragma once
#ifndef __SDL_SYSTEM_SPATIAL_INDEX_H__
#define __SDL_SYSTEM_SPATIAL_INDEX_H__

#include "page_head.h"
#include "spatial_type.h"

namespace sdl { namespace db {

#pragma pack(push, 1) 

struct spatial_root_row_20 {
    
    // m_pageId = (1:16708)
    // pminlen = 20

    // Record Type = INDEX_RECORD 
    // Record Attributes = EMPTY
    // FileId = 1
    // PageId = 16708
    // Row = 0
    // Level = 2
    // ChildFileId = 1
    // ChildPageId = 1122
    // Cell_id (key) = NULL
    // pk0 (key) = NULL
    // KeyHashValue = NULL
    // Row Size = 20

    //06000000 00000000 08000000 00006204 00000100

    struct data_type { // Record Size = 20
        uint8       _0x00[14];      // 0x00 : 14 bytes             // 06000000 00000000 08000000 0000
        pageFileID  page;           // 0x0e : 6 bytes = [1:1122]   // 6204 00000100
    };
    union {
        data_type data;
        char raw[sizeof(data_type)];
    };
};

struct spatial_root_row_23 {

    // Slot 0 Offset 0x60 Length 23
    // Record Type = INDEX_RECORD
    // Record Attributes =  NULL_BITMAP
    // Record Size = 23
    // FileId = 1
    // PageId = 726466
    // Row = 0
    // Level = 2
    // ChildFileId = 1
    // ChildPageId = 726464
    // Cell_id (key) = NULL
    // pk0 (key) = NULL
    // KeyHashValue = NULL
    // Row Size = 23

    //0000000000000000:   165bfc00 00034c06 00000000 0000c015 0b000100 
    //0000000000000014:   020000 

    struct data_type { // Record Size = 23
        uint8       _0x00[14];      // 0x00 : 14 bytes              // 165bfc00 00034c06 00000000 0000
        pageFileID  page;           // 0x0e : 6 bytes = [1:726464]  // c015 0b000100
        uint8       _0x14[3];       // 0x14 : 3 bytes               // 020000
    };
    union {
        data_type data;
        char raw[sizeof(data_type)];
    };
};

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

struct spatial_page_row_meta;
struct spatial_page_row_info;

struct spatial_page_row { //spatial_leaf_row
    
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

struct geo_point_meta;
struct geo_point_info;

struct geo_point { // 22 bytes
    
    using meta = geo_point_meta;
    using info = geo_point_info;

    struct data_type {
        uint32  SRID;       // 0x00 : 4 bytes // E6100000 = 4326 (WGS84 — SRID 4326)
        uint16  _0x04;      // 0x04 : 2 bytes // 3073 = 0xC01
        double  latitude;   // 0x06 : 8 bytes // 404dc500e6afcce2 = 59.53909 (POINT_Y) (Lat)
        double  longitude;  // 0x0e : 8 bytes // 4062dc587d6f9768 = 150.885802 (POINT_X) (Lon)   
    };
    union {
        data_type data;
        char raw[sizeof(data_type)];
    };
};

struct geo_multipolygon_meta;
struct geo_multipolygon_info;

struct geo_multipolygon { // = 26 bytes

    using meta = geo_multipolygon_meta;
    using info = geo_multipolygon_info;

    struct data_type {
        uint32  SRID;               // 0x00 : 4 bytes // E6100000 = 4326 (WGS84 — SRID 4326)
        uint16  _0x04;              // 0x04 : 2 bytes // 0104 = ?
        uint32  num_point;          // 0x06 : 4 bytes // EC010000 = 0x01EC = 492 = POINTS COUNT
        spatial_point points[1];    // 0x0A : 16 bytes * point_num
    };
    union {
        data_type data;
        char raw[sizeof(data_type)];
    };
    size_t size() const { 
        return data.num_point;
    } 
    spatial_point const & operator[](size_t i) const {
        SDL_ASSERT(i < this->size());
        return data.points[i];
    }
    spatial_point const * begin() const {
        return data.points;
    }
    spatial_point const * end() const {
        return data.points + this->size();
    }
    size_t mem_size() const {
        return sizeof(data_type)-sizeof(spatial_point)+sizeof(spatial_point)*size();
    }
};

//struct geo_linestring {};
//struct geo_polygon {};

#pragma pack(pop)

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

struct geo_point_meta: is_static {

    typedef_col_type_n(geo_point, SRID);
    typedef_col_type_n(geo_point, _0x04);
    typedef_col_type_n(geo_point, latitude);
    typedef_col_type_n(geo_point, longitude);

    typedef TL::Seq<
        SRID
        ,_0x04
        ,latitude
        ,longitude
    >::Type type_list;
};

struct geo_point_info: is_static {
    static std::string type_meta(geo_point const &);
    static std::string type_raw(geo_point const &);
};

//------------------------------------------------------------------------

struct geo_multipolygon_meta: is_static {

    typedef_col_type_n(geo_multipolygon, SRID);
    typedef_col_type_n(geo_multipolygon, _0x04);
    typedef_col_type_n(geo_multipolygon, num_point);

    typedef TL::Seq<
        SRID
        ,_0x04
        ,num_point
    >::Type type_list;
};

struct geo_multipolygon_info: is_static {
    static std::string type_meta(geo_multipolygon const &);
    static std::string type_raw(geo_multipolygon const &);
};

} // db
} // sdl

#endif // __SDL_SYSTEM_SPATIAL_INDEX_H__