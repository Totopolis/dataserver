// usertable.h
//
#ifndef __SDL_SYSTEM_USERTABLE_H__
#define __SDL_SYSTEM_USERTABLE_H__

#pragma once

#include "datapage.h"

namespace sdl { namespace db {

class usertable : noncopyable
{
public:
    class column {
    public:
        std::string name;
        scalartype type;
        scalarlen length; //  -1 if this is a varchar(max) / text / image data type with no practical maximum length

        column(syscolpars_row const *,
               sysscalartypes_row const *, 
               std::string && _name);
    };
public:
    using columns = std::vector<column>;

    sysschobjs_row const * const schobj; // id, name
    const std::string name; 
    const columns cols;

    usertable(sysschobjs_row const *, std::string && _name, columns && );

    schobj_id get_id() const {
        return schobj->data.id;
    }
    std::string type_schema() const;
};

} // db
} // sdl

#endif // __SDL_SYSTEM_USERTABLE_H__
