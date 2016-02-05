// usertable.h
//
#ifndef __SDL_SYSTEM_USERTABLE_H__
#define __SDL_SYSTEM_USERTABLE_H__

#pragma once

#include "datapage.h"

namespace sdl { namespace db {

class tablecolumn 
{
public:
    const std::string name;
    const scalartype type;
    const scalarlen length; //  -1 if this is a varchar(max) / text / image data type with no practical maximum length

    bool is_varlength() const {
        return this->length.is_var();
    }
    tablecolumn(syscolpars_row const *,
                sysscalartypes_row const *, 
                const std::string & _name);
};

class usertable : noncopyable
{
    sysschobjs_row const * const schobj; // id, name
public:
    using columns = std::vector<tablecolumn>;
    const schobj_id id;
    const std::string name; 
    const columns cols;

    usertable(sysschobjs_row const *, std::string && _name, columns && );

    std::string type_schema() const;
};

} // db
} // sdl

#endif // __SDL_SYSTEM_USERTABLE_H__
