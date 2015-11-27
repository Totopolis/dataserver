// syschobjs.h
//
#ifndef __SDL_SYSTEM_SYSCHOBJS_H__
#define __SDL_SYSTEM_SYSCHOBJS_H__

#pragma once

#include "page_head.h"

namespace sdl { namespace db {

// System Table: syschobjs(ObjectID = 34)
// The sysschobjs table is the underlying table for the sys.objects table.
// It has a NULL bitmap and one variable length column.
// Note that there is also a sys.system_objects table with object IDs less than 0,
// but the objects shown in that view are not in this table.

struct syschobjs
{
    //FIXME:  to be tested
    struct data_type {
        uint32      id;         // id(object_id) - 4 bytes - the unique ID for the object.
                                // This will be the same as the allocation unit's object ID for system objects, 
                                // but otherwise it will be a random number between 100 and 2^31.
        uint32      nsid;       // nsid(schema_id) - 4 bytes - the schema ID of this object.
        uint8       nsclass;    // nsclass - 1 byte - this is not shown in the DMV
        uint32      status;     // status - 4 bytes - this is not shown in the DMV
        uint16      type;       // type(type) - 2 bytes, char(2) - this is the type of the object
        uint32      pid;        // pid(parent_object_id) - 4 bytes - if this object belongs to a table, then this is the ObjectID of the table it belongs to.
        uint8       pclass;     // pclass - 1 byte
        uint32      intprop;    // intprop - 4 bytes
        uint64      created;    // created(create_date) - 8 bytes, datetime - the time the object was first created.
        uint64      modified;   // modified(modify_date) - 8 bytes, datetime - the time the schema for this object was last modified.

        /*FIXME: name (name) - the name of the object, Unicode, variable length. 
        The (name, schema) must be unique among all objects in a database.*/
    };
    data_type data;
};

} // db
} // sdl

#endif // __SDL_SYSTEM_SYSCHOBJS_H__