// sysallocunits.h
//
#ifndef __SDL_SYSTEM_SYSALLOCUNITS_H__
#define __SDL_SYSTEM_SYSALLOCUNITS_H__

#pragma once

#include "page_head.h"

namespace sdl { namespace db {

#pragma pack(push, 1) 

// System Table: sysallocunits (ObjectID = 7)
// The sysallocunits table is the entry point containing the metadata that describes all other tables in the database.
// The first page of this table is pointed to by the dbi_firstSysIndexes field on the boot page.
// The records in this table have 12 fixed length columns, a NULL bitmap, and a number of columns field.
// Most of the columns in this table can be seen through the DMVs sys.system_internals_allocation_units, 
// sys.partitions, sys.dm_db_partition_stats, and sys.allocation_units.
// The names in parenthesis are the columns as they appear in the DMV.

struct sysallocunits_row
{
    // The sysallocunits table is the entry point containing the metadata that describes all other tables in the database. 
    // The first page of this table is pointed to by the dbi_firstSysIndexes field on the boot page. 
    // The records in this table have 12 fixed length columns, a NULL bitmap, and a number of columns field.
    struct data_type {

        row_head        head;           // 4 bytes
        auid_t          auid;           // auid(allocation_unit_id / partition_id) - 8 bytes - the unique ID / primary key for this allocation unit.
        uint8           type;           // type(type) - 1 byte - 1 = IN_ROW_DATA, 2 = LOB_DATA, 3 = ROW_OVERFLOW_DATA
        uint64          ownerid;        // ownerid(container_id / hobt_id) - 8 bytes - this is usually also an auid value, but sometimes not.
        uint32          status;         // status - 4 bytes - this column is not shown directly in any of the DMVs.
        uint16          fgid;           // fgid(filegroup_id) - 2 bytes
        pageFileID      pgfirst;        // pgfirst(first_page) - 6 bytes - page locator of the first data page for this allocation unit.
                                        // If this allocation unit has no data, then this will be 0:0, as will pgroot and pgfirstiam.
        pageFileID      pgroot;         // pgroot(root_page) - 6 bytes - page locator of the root page of the index tree, if this allocation units is for a B - tree.
        pageFileID      pgfirstiam;     // pgfirstiam(first_iam_page) - 6 bytes - page locator of the first IAM page for this allocation unit.
        uint64          pcused;         // pcused(used_pages) - 8 bytes - used page count - this is the number of data pages plus IAM and index pages(PageType IN(2, 10))
        uint64          pcdata;         // pcdata(data_pages) - 8 bytes - data page count - the numbr of pages specifically for data only(PageType IN(1, 3, 4))
        uint64          pcreserved;     // pcreserved(total_pages) - 8 bytes - reserved page count - this is the total number of pages used plus pages not yet used but reserved for future use.Reserving pages ahead of time is probably done to reduce locking time when more space needs to be allocated.
        uint32          dbfragid;       // dbfragid - 4 bytes - this column is not shown in the DMV
    };
    union {
        data_type data;
        char raw[sizeof(data_type)];
    };
    const char * type_name() const;
};

#pragma pack(pop)

template<> struct null_bitmap_traits<sysallocunits_row> {
    enum { value = 1 };
};

struct sysallocunits_row_meta {

    typedef_col_type_n(sysallocunits_row, head);
    typedef_col_type_n(sysallocunits_row, auid);
    typedef_col_type_n(sysallocunits_row, type);
    typedef_col_type_n(sysallocunits_row, ownerid);
    typedef_col_type_n(sysallocunits_row, status);
    typedef_col_type_n(sysallocunits_row, fgid);
    typedef_col_type_n(sysallocunits_row, pgfirst);
    typedef_col_type_n(sysallocunits_row, pgroot);
    typedef_col_type_n(sysallocunits_row, pgfirstiam);
    typedef_col_type_n(sysallocunits_row, pcused);
    typedef_col_type_n(sysallocunits_row, pcdata);
    typedef_col_type_n(sysallocunits_row, pcreserved);
    typedef_col_type_n(sysallocunits_row, dbfragid);

    typedef TL::Seq<
        head
        ,auid
        ,type
        ,ownerid
        ,status
        ,fgid
        ,pgfirst
        ,pgroot
        ,pgfirstiam
        ,pcused
        ,pcdata
        ,pcreserved
        ,dbfragid
    >::Type type_list;

    sysallocunits_row_meta() = delete;
};

struct sysallocunits_row_info {
    sysallocunits_row_info() = delete;
    static std::string type_meta(sysallocunits_row const &);
    static std::string type_raw(sysallocunits_row const &);
};

} // db
} // sdl

#endif // __SDL_SYSTEM_SYSALLOCUNITS_H__