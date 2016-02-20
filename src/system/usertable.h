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

        column(syscolpars_row const *, sysscalartypes_row const *);

        bool is_fixed() const;

        size_t fixed_size() const {
            SDL_ASSERT(is_fixed());
            return length._16;
        }
    };
    using column_ref = column const &;
    using columns = std::vector<std::unique_ptr<column>>;
public:
    sysschobjs_row const * const schobj;
    usertable(sysschobjs_row const *, columns && );

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
            if (fun(*c)) ++ret;
        }
        return ret;
    }
    template<class fun_type>
    size_t find_if(fun_type fun) const {
        size_t ret = 0;
        for (auto const & c : m_schema) {
            if (fun(*c)) break;
            ++ret;
        }
        return ret;
    }
    size_t count_var() const;       // # of variable cols
    size_t count_fixed() const;     // # of fixed cols
    size_t fixed_size() const;
    size_t fixed_offset(size_t) const;
    size_t var_offset(size_t) const;

    template<typename... Ts> static
    void emplace_back(columns & cols, Ts&&... params) {
        cols.push_back(sdl::make_unique<column>(std::forward<Ts>(params)...));
    }
private:
    void init_offset();
    const std::string m_name; 
    const columns m_schema;
    std::vector<size_t> m_offset; // fixed columns offset
};

} // db
} // sdl

#endif // __SDL_SYSTEM_USERTABLE_H__
