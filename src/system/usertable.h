// usertable.h
//
#ifndef __SDL_SYSTEM_USERTABLE_H__
#define __SDL_SYSTEM_USERTABLE_H__

#pragma once

#include "datapage.h"

namespace sdl { namespace db {

class usertable : noncopyable {
public:
    class column : noncopyable {
        friend usertable;
        syscolpars_row const * const colpar;
        sysscalartypes_row const * const scalar;
    public:
        const std::string name;
        const scalartype::type type;
        const scalarlen length; //  -1 if this is a varchar(max) / text / image data type with no practical maximum length

        column(syscolpars_row const *,
               sysscalartypes_row const *, 
               std::string && _name);

        bool is_fixed() const;
        size_t fixed_size() const;
    };
    using column_ref = column const &;
    using columns = std::vector<std::unique_ptr<column>>;
public:
    sysschobjs_row const * const schobj;
    usertable(sysschobjs_row const *, std::string && _name, columns && );

    schobj_id get_id() const {
        return schobj->data.id;
    }
    const std::string & name() const {
        return m_name;
    }
    const columns & schema() const {
        return m_schema;
    }
    size_t size() const {
        return m_schema.size();
    }
    column_ref operator[](size_t i) const {
        return *m_schema[i];
    }
    std::string type_schema() const;

    template<class fun_type>
    void for_col(fun_type fun) const {
        for (auto const & c : m_schema) {
            fun(*c);
        }
    }
    template<class fun_type>
    size_t count_if(fun_type fun) const {
        size_t ret = 0;
        for (auto const & c : m_schema) {
            if (fun(*c))
                ++ret;
        }
        return ret;
    }
    size_t count_var() const;
    size_t count_fixed() const;
    size_t fixed_offset(size_t) const;
    size_t fixed_size() const;
private:
    void init_offset();
    const std::string m_name; 
    const columns m_schema;
    std::vector<size_t> m_offset; // for fixed
};

template<typename... Ts> inline
void emplace_back(usertable::columns & cols, Ts&&... params) {
    cols.push_back(sdl::make_unique<usertable::column>(std::forward<Ts>(params)...));

}

} // db
} // sdl

#endif // __SDL_SYSTEM_USERTABLE_H__
