// sysschobjs.h
//
#ifndef __SDL_SYSTEM_SYSSCHOBJS_H__
#define __SDL_SYSTEM_SYSSCHOBJS_H__

#pragma once

#include "page_head.h"

namespace sdl { namespace db {

#pragma pack(push, 1) 

struct sysschobjs_row_meta;
struct sysschobjs_row_info;

// System Table: sysschobjs(ObjectID = 34)
// The sysschobjs table is the underlying table for the sys.objects table.
// It has a NULL bitmap and one variable length column.
// Note that there is also a sys.system_objects table with object IDs less than 0,
// but the objects shown in that view are not in this table.

struct sysschobjs_row
{
    using meta = sysschobjs_row_meta;
    using info = sysschobjs_row_info;

    enum { dump_raw = 0x78 };  // temporal

    struct data_type
    {
        row_head        head;       // 4 bytes
        schobj_id       id;         // id(object_id) - 4 bytes - the unique ID for the object.
                                    // This will be the same as the allocation unit's object ID for system objects, 
                                    // but otherwise it will be a random number between 100 and 2^31.
        uint32          nsid;       // nsid(schema_id) - 4 bytes - the schema ID of this object.
        uint8           nsclass;    // nsclass - 1 byte - this is not shown in the DMV
        uint32          status;     // status - 4 bytes - this is not shown in the DMV
        obj_code        type;       // type(type) - 2 bytes, char(2) - this is the type of the object
        uint32          pid;        // pid(parent_object_id) - 4 bytes - if this object belongs to a table, then this is the ObjectID of the table it belongs to.
        uint8           pclass;     // pclass - 1 byte
        uint32          intprop;    // intprop - 4 bytes
        datetime_t      created;    // created(create_date) - 8 bytes, datetime - the time the object was first created.
        datetime_t      modified;   // modified(modify_date) - 8 bytes, datetime - the time the schema for this object was last modified.
    };
    union {
        data_type data;
        char raw[sizeof(data_type) > dump_raw ? sizeof(data_type) : dump_raw];
    };

    bool is_type(obj_code::type t) const {
        return obj_code::get_code(t) == data.type;
    }
    bool is_USER_TABLE() const {
        return is_type(obj_code::type::USER_TABLE);
    }
    bool is_USER_TABLE_id() const {
        return is_USER_TABLE() && (this->data.id._32 > 0);
    }
};

#pragma pack(pop)

template<> struct null_bitmap_traits<sysschobjs_row> {
    enum { value = 1 };
};

template<> struct variable_array_traits<sysschobjs_row> {
    enum { value = 1 };
};

struct sysschobjs_row_meta: is_static {

    typedef_col_type_n(sysschobjs_row, head);
    typedef_col_type_n(sysschobjs_row, id);
    typedef_col_type_n(sysschobjs_row, nsid);
    typedef_col_type_n(sysschobjs_row, nsclass);
    typedef_col_type_n(sysschobjs_row, status);
    typedef_col_type_n(sysschobjs_row, type);
    typedef_col_type_n(sysschobjs_row, pid);
    typedef_col_type_n(sysschobjs_row, pclass);
    typedef_col_type_n(sysschobjs_row, intprop);
    typedef_col_type_n(sysschobjs_row, created);
    typedef_col_type_n(sysschobjs_row, modified);

    typedef_var_col(0, nchar_range, name);

    typedef TL::Seq<
        head
        ,id
        ,nsid
        ,nsclass
        ,status
        ,type
        ,pid
        ,pclass
        ,intprop
        ,created
        ,modified
        ,name
    >::Type type_list;
};

struct sysschobjs_row_info: is_static {
    static std::string type_meta(sysschobjs_row const &);
    static std::string type_raw(sysschobjs_row const &);
    static std::string col_name(sysschobjs_row const &);
};

} // db
} // sdl

#endif // __SDL_SYSTEM_SYSSCHOBJS_H__