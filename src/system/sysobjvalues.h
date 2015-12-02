// sysobjvalues.h
//
#ifndef __SDL_SYSTEM_SYSOBJVALUES_H__
#define __SDL_SYSTEM_SYSOBJVALUES_H__

#pragma once

#include "page_type.h"

namespace sdl { namespace db {

#pragma pack(push, 1) 

/*System Table: sysobjvalues (ObjectID = 60)

The sysobjvalues table has a NULL bitmap and two variable length columns :

valclass - 1 byte
objid(object_id) - 4 bytes
subobjid - 4 bytes
valnum - 4 bytes
value - 6 bytes - seems to have object specific data indicating how the imageval should be interpreted
imageval(definition) - image(up to 1GB).For non - CLR procedure, function, view, trigger, 
                        and constraint object types(C, D, FN, IF, P, PG, TF, TR, V), 
                        this is the T - SQL code of the object as varchar data.
*/
struct sysobjvalues
{
    //FIXME: to be tested
    struct data_type {

         
    };
    data_type data;
};

#pragma pack(pop)

} // db
} // sdl

#endif // __SDL_SYSTEM_SYSOBJVALUES_H__