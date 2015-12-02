// sysscalartypes.h
//
#ifndef __SDL_SYSTEM_SYSSCALARTYPES_H__
#define __SDL_SYSTEM_SYSSCALARTYPES_H__

#pragma once

#include "page_type.h"

namespace sdl { namespace db {

#pragma pack(push, 1) 

/* (ObjectID = 50)
The sysscalartypes table holds a row for every built-in and user-defined data type. 
It can be read using the DMVs sys.systypes and sys.types. */

struct sysscalartypes
{
    //FIXME: to be tested
    struct data_type {

        // id - 4 bytes - the unique id for this built-in type or UDT.
        uint32 id;

        // schid - 4 bytes - the schema that owns this data type.
        uint32 schid;

        /* xtype - 1 byte - the same as the xtype values in the syscolpars table -
        equal to the id for built-in types. */
        uint8 xtype;

        // length - 2 bytes - the length of the data type in bytes
        uint16 length;

        // prec - 1 byte
        uint8 prec;

        // scale - 1 byte
        uint8 scale;

        // collationid - 4 bytes
        uint32 collationid;

        /* status - 4 bytes - status flags about the type.  If 0x1 is set, the type does not
        allow NULLs (default is to allow NULLs) */
        uint32 status;

        // created - datetime, 8 bytes
        uint64 created;

        // modified - datetime, 8 bytes
        uint64 modified;

        // dflt - 4 bytes
        uint32 dflt;

        // chk - 4 bytes
        uint32 chk;

    };
    data_type data;
};

#pragma pack(pop)

} // db
} // sdl

#endif // __SDL_SYSTEM_SYSSCALARTYPES_H__

