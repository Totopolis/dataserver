// sysiscols.h
//
#ifndef __SDL_SYSTEM_SYSISCOLS_H__
#define __SDL_SYSTEM_SYSISCOLS_H__

#pragma once

#include "page_head.h"

namespace sdl { namespace db {

#pragma pack(push, 1) 

/*System Table: sysiscols (ObjectID = 55)
The sysiscols is a table with only 22 bytes of fixed length data per row, 
and it defines all indexes and statistics in system and user tables, 
both clustered and non-clustered indexes. 
Heaps (index id = 0) are not included in this table. 
The data in this table is visible via the DMV sys.index_columns (indexes only, statistics columns are not included). 
This table is clustered by (idmajor, idminor, subid):
*/
struct sysiscols_row
{
    enum { dump_raw = 0x28 };  // temporal

    //FIXME: to be tested
    struct data_type {

        record_head head;   // 4 bytes

        int32   idmajor;    // (object_id) - 4 bytes - the object_id of the object that the index is defined on
        int32   idminor;    // (index_id) - 4 bytes - the index id or statistics id for each object_id
        int32   subid;      // (index_column_id) - 4 bytes - when an index contains multiple columns, this is the order of the columns within the index.For statistics, this appears to always be 1
        uint32  status;     // 4 bytes - bit mask : 0x1 appears to always be set, 0x2 for index, 0x4 for a descending index column(is_descending_key).
        int32   intprop;    // (column_id) - 4 bytes - appears to the column(syscolpars.colid)
        uint8   tinyprop1;  // 1 byte - appears to be equal to the subid for an index, 0 for a statistic.
        uint8   tinyprop2;  // 1 byte - appears to always be 0
    };
    union {
        data_type data;
        char raw[sizeof(data_type) > dump_raw ? sizeof(data_type) : dump_raw];
    };
};

#pragma pack(pop)

struct sysiscols_meta {

    typedef_col_type_n(sysiscols_row, head);
    typedef_col_type_n(sysiscols_row, idmajor);
    typedef_col_type_n(sysiscols_row, idminor);
    typedef_col_type_n(sysiscols_row, subid);
    typedef_col_type_n(sysiscols_row, status);
    typedef_col_type_n(sysiscols_row, intprop);
    typedef_col_type_n(sysiscols_row, tinyprop1);
    typedef_col_type_n(sysiscols_row, tinyprop2);

    typedef TL::Seq<
        head
        ,idmajor
        ,idminor
        ,subid
        ,status
        ,intprop
        ,tinyprop1
        ,tinyprop2
    >::Type type_list;

    sysiscols_meta() = delete;
};

struct sysiscols_row_info {
    sysiscols_row_info() = delete;
    static std::string type_meta(sysiscols_row const &);
    static std::string type_raw(sysiscols_row const &);
};

} // db
} // sdl

#endif // __SDL_SYSTEM_SYSISCOLS_H__