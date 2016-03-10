// sysobjvalues.h
//
#ifndef __SDL_SYSOBJ_SYSOBJVALUES_H__
#define __SDL_SYSOBJ_SYSOBJVALUES_H__

#pragma once

#include "system/page_head.h"

namespace sdl { namespace db {

#pragma pack(push, 1) 

struct sysobjvalues_row_meta;
struct sysobjvalues_row_info;

/*System Table: sysobjvalues (ObjectID = 60)
The sysobjvalues table has a NULL bitmap and two variable length columns :
*/
struct sysobjvalues_row
{
    using meta = sysobjvalues_row_meta;
    using info = sysobjvalues_row_info;

    /*struct _48 // 6 bytes 
    {
        uint32 _32;     // 4 bytes
        uint16 _16;     // 2 bytes
    };*/

    struct data_type {

        row_head head;   // 4 bytes

        uint8   valclass;   // 1 byte
        int32   objid;      // 4 bytes
        int32   subobjid;   // 4 bytes
        int32   valnum;     // 4 bytes
        
        // can be number, INVALID COLUMN VALUE, [NULL] 
        //_48     value;      // 6 bytes - seems to have object specific data indicating how the imageval should be interpreted

        /* imageval(definition) - image(up to 1GB).
        For non - CLR procedure, function, view, trigger, 
        and constraint object types(C, D, FN, IF, P, PG, TF, TR, V), 
        this is the T - SQL code of the object as varchar data.
        */
    };
    union {
        data_type data;
        char raw[sizeof(data_type)];
    };
};

#pragma pack(pop)

template<> struct null_bitmap_traits<sysobjvalues_row> {
    enum { value = 1 };
};

/*template<> struct variable_array_traits<sysobjvalues_row> {
    enum { value = 1 };
};*/

struct sysobjvalues_row_meta: is_static {

    typedef_col_type_n(sysobjvalues_row, head);
    typedef_col_type_n(sysobjvalues_row, valclass);
    typedef_col_type_n(sysobjvalues_row, objid);
    typedef_col_type_n(sysobjvalues_row, subobjid);
    typedef_col_type_n(sysobjvalues_row, valnum);
    //typedef_col_type_n(sysobjvalues_row, value);

    typedef TL::Seq<
        head
        ,valclass
        ,objid
        ,subobjid
        ,valnum
        //,value
    >::Type type_list;
};

struct sysobjvalues_row_info: is_static {
    static std::string type_meta(sysobjvalues_row const &);
    static std::string type_raw(sysobjvalues_row const &);
};

} // db
} // sdl

#endif // __SDL_SYSOBJ_SYSOBJVALUES_H__