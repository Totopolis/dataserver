// sysrowsets.h
//
#pragma once
#ifndef __SDL_SYSOBJ_SYSROWSETS_H__
#define __SDL_SYSOBJ_SYSROWSETS_H__

#include "dataserver/system/page_head.h"

namespace sdl { namespace db {

#pragma pack(push, 1) 

struct sysrowsets_row_meta;
struct sysrowsets_row_info;

// System Table: sysrowsets (ObjectID = 5)
// Contains a row for each partition rowset for an index or a heap.  
struct sysrowsets_row
{
    using meta = sysrowsets_row_meta;
    using info = sysrowsets_row_info;

    struct data_type { // to be tested

        row_head    head;           // 4 bytes
        auid_t      rowsetid;       // PartitionID/HobtID = sysallocunits_row.ownerid
        uint8       ownertype;
        schobj_id   idmajor;        // ObjectID = sysschobjs_row.id = usertable.get_id()
        uint32      idminor;        // IndexID
        uint32      numpart;        // PartitionNumber
        uint32      status;
        uint16      fgidfs;         // FilestreamFilegroupID
        uint64      rcrows;
        uint8       cmprlevel;      // DataCompression
        uint8       fillfact;
        uint16      maxnullbit;
        uint32      maxleaf;
        uint16      maxint;
        uint16      minleaf;
        uint16      minint;
    };
    union {
        data_type data;
        char raw[sizeof(data_type)];
    };
};

#pragma pack(pop)

struct sysrowsets_row_meta: is_static {

    typedef_col_type_n(sysrowsets_row, head);
    typedef_col_type_n(sysrowsets_row, rowsetid);
    typedef_col_type_n(sysrowsets_row, ownertype);
    typedef_col_type_n(sysrowsets_row, idmajor);
    typedef_col_type_n(sysrowsets_row, idminor);
    typedef_col_type_n(sysrowsets_row, numpart);
    typedef_col_type_n(sysrowsets_row, status);
    typedef_col_type_n(sysrowsets_row, fgidfs);
    typedef_col_type_n(sysrowsets_row, rcrows);
    typedef_col_type_n(sysrowsets_row, cmprlevel);
    typedef_col_type_n(sysrowsets_row, fillfact);
    typedef_col_type_n(sysrowsets_row, maxnullbit);
    typedef_col_type_n(sysrowsets_row, maxleaf);
    typedef_col_type_n(sysrowsets_row, maxint);
    typedef_col_type_n(sysrowsets_row, minleaf);
    typedef_col_type_n(sysrowsets_row, minint);

    typedef TL::Seq<
        head
        ,rowsetid
        ,ownertype
        ,idmajor
        ,idminor
        ,numpart
        ,status
        ,fgidfs
        ,rcrows
        ,cmprlevel
        ,fillfact
        ,maxnullbit
        ,maxleaf
        ,maxint
        ,minleaf
        ,minint
    >::Type type_list;
};

struct sysrowsets_row_info: is_static {
    static std::string type_meta(sysrowsets_row const &);
    static std::string type_raw(sysrowsets_row const &);
};

} // db
} // sdl

#endif // __SDL_SYSOBJ_SYSROWSETS_H__