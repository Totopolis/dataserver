// spatial_index.h
//
#pragma once
#ifndef __SDL_SYSTEM_SPATIAL_INDEX_H__
#define __SDL_SYSTEM_SPATIAL_INDEX_H__

#include "page_head.h"

namespace sdl { namespace db { namespace todo {

#pragma pack(push, 1) 

struct spatial_cell { // 5 bytes

    struct data_type { //64 0C 52 9B 04 
        uint8 a;
        uint8 b;
        uint8 c;
        uint8 d;
        uint8 e;
    };
    union {
        data_type data;
        uint8 id[5];
        char raw[sizeof(data_type)];
    };
};

struct spatial_root_row {
    
    // Record Type = INDEX_RECORD 
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
        pageFileID  page;           // 0x10 : 6 bytes = [1:1122]   // 6204 00000100
    };
    union {
        data_type data;
        char raw[sizeof(data_type)];
    };
};

struct spatial_node_row {

    // Record Type = INDEX_RECORD
    // Record Attributes =  NULL_BITMAP
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
    
    struct data_type {  // Record Size = 23 
        bitmask8        statusA;            // 0x00 : 1 byte = 0x16                 // 16
        spatial_cell    cell_id;            // 0x01 : 5 bytes = 0x640C529B04        // 640c52 9b04
        uint8           pk0[8];             // 0x06 : 8 bytes = 1469437 = 0x166BFD  // fd6b 16000000 0000
        pageFileID      page;               // 0x0e : 6 bytes = [1:469608]          // 682a 07000100
        uint8           _0x14[3];           // 0x14 : 3 bytes                       // 020000
    };
    union {
        data_type data;
        char raw[sizeof(data_type)];
    };
};

struct spatial_leaf_row {

    // 10001700 60985955 04009e1f 00000000 000100e6 10000004 0000

    struct data_type { // Record Size = 26
        uint8           _0x00[4];       // 0x00 : 4 bytes                       // 10001700
        spatial_cell    cell_id;        // 0x04 : 5 bytes = 0x6098595504        // 60985adf 04          // Column 1 Offset 0x4 Length 5 Length (physical) 5
        uint8           pk0[8];         // 0x09 : 8 bytes = 2072064 = 0x1F9E00  // 009e1f 00000000 00   // Column 4 Offset 0x9 Length 8 Length (physical) 8
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

} // todo
} // db
} // sdl

#endif // __SDL_SYSTEM_SPATIAL_INDEX_H__