// syscolpars.h
//
#ifndef __SDL_SYSTEM_SYSCOLPARS_H__
#define __SDL_SYSTEM_SYSCOLPARS_H__

#pragma once

#include "page_type.h"

namespace sdl { namespace db {

#pragma pack(push, 1) 

// System Table: syschobjs(ObjectID = 34)
// The sysschobjs table is the underlying table for the sys.objects table.
// It has a NULL bitmap and one variable length column.
// Note that there is also a sys.system_objects table with object IDs less than 0,
// but the objects shown in that view are not in this table.

struct syscolpars
{
    //FIXME: to be tested
    struct data_type {
          
        /*4 bytes - the ObjectID of the table or view that owns this object*/
        uint32 id;

        /*number (number) - 2 bytes - for functions and procedures,
        this will be 1 for input parameters, it is 0 for output columns of table-valued functions and tables and views*/
        uint16 number;

        /*colid (colid) - 4 bytes - the unique id of the column within this object,
        starting at 1 and counting upward in sequence.  (id, colid, number) is
        unique among all columns in the database.*/
        uint32 colid;

        /*xtype (xtype) - 1 byte - an ID for the data type of this column.
        This references the system table sys.sysscalartypes.xtype.*/
        uint8 xtype;

        /*utype (xusertype) - 4 bytes - usually equal to xtype,
        except for user defined types and tables.  This references
        the system table sys.sysscalartypes.id*/
        uint32 utype;

        /*length (length) - 2 bytes - length of this column in bytes,
        -1 if this is a varchar(max) / text / image data type with no practical maximum length.*/
        uint16 length;

        // prec (prec) - 1 byte
        uint8 prec;

        // scale (scale) - 1 byte
        uint8 scale;

        /*collationid (collationid) - 4 bytes */
        uint32 collationid;

        /*status (status) - 4 bytes*/
        uint32 status;

        // maxinrow - 2 bytes
        uint16 maxinrow;

        // xmlns - 4 bytes
        uint32 xmlns;

        // dflt - 4 bytes
        uint32 dflt;

        // chk - 4 bytes
        uint32 chk;

        // TODO: RawType.VarBinary("idtval")
        // idtval - variable length?

        /*name (name) - variable length, nvarchar - the name of this column.*/
    };
    data_type data;
};

#pragma pack(pop)

} // db
} // sdl

#endif // __SDL_SYSTEM_SYSCOLPARS_H__