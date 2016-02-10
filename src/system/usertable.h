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
        scalartype::type type;
        scalarlen length; //  -1 if this is a varchar(max) / text / image data type with no practical maximum length

        column(syscolpars_row const *,
               sysscalartypes_row const *, 
               std::string && _name);

        bool is_fixed() const;
        size_t fixed_size() const;
    };
    using column_ref = column const &;
    using columns = std::vector<column>;
public:
    sysschobjs_row const * const schobj; // id, name
    const std::string name; 
    const columns schema; // cols

    usertable(sysschobjs_row const *, std::string && _name, columns && );

    schobj_id get_id() const {
        return schobj->data.id;
    }
    std::string type_schema() const;

    template<class fun_type>
    size_t count_if(fun_type fun) const {
        size_t ret = 0;
        for (auto const & c : schema) {
            if (fun(c))
                ++ret;
        }
        return ret;
    }
    size_t count_var() const;
    size_t count_fixed() const;
    size_t fixed_offset(size_t) const;
private:
    void init_offset();
    std::vector<size_t> column_offset; // for fixed
};

} // db
} // sdl

#endif // __SDL_SYSTEM_USERTABLE_H__
