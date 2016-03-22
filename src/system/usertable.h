// usertable.h
//
#ifndef __SDL_SYSTEM_USERTABLE_H__
#define __SDL_SYSTEM_USERTABLE_H__

#pragma once

#include "datapage.h"

namespace sdl { namespace db {

class primary_key;

class usertable : noncopyable {
    using usertable_error = sdl_exception_t<usertable>;
public:
    class column : noncopyable {
    public:
        friend usertable;
        syscolpars_row const * const colpar;
        sysscalartypes_row const * const scalar;
    public:
        const std::string name;
        const scalartype::type type;
        const scalarlen length; //  -1 if this is a varchar(max) / text / image data type with no practical maximum length

        column(syscolpars_row const *, sysscalartypes_row const *);
        static bool is_fixed(syscolpars_row const *, sysscalartypes_row const *);

        bool is_fixed() const;
        bool is_array() const;

        size_t fixed_size() const {
            SDL_ASSERT(is_fixed());
            return length._16;
        }
        std::string type_schema(primary_key const * PK = nullptr) const;
    };
    using column_ref = column const &;
    using columns = std::vector<std::unique_ptr<column>>;
public:
    sysschobjs_row const * const schobj;
    usertable(sysschobjs_row const *, columns &&, primary_key const *);

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

    std::string type_schema(primary_key const * PK = nullptr) const;

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
    template<class fun_type>
    size_t find_if(fun_type fun) const {
        size_t ret = 0;
        for (auto const & c : m_schema) {
            if (fun(*c))
                break;
            ++ret;
        }
        return ret; // return size() if column not found
    }
    using col_index = std::pair<column const *, size_t>;
    col_index find_col(syscolpars_row const *) const;
public:
    size_t count_var() const;       // # of variable cols
    size_t count_fixed() const;     // # of fixed cols
    size_t fixed_size() const;
    size_t fixed_offset(size_t) const;
    size_t var_offset(size_t) const;
    size_t offset(size_t i) const {
        return m_offset[i];
    }
    bool is_fixed(size_t i) const {
        return (*this)[i].is_fixed();
    }
    template<typename... Ts> static
    void emplace_back(columns & cols, Ts&&... params) {
        cols.push_back(sdl::make_unique<column>(std::forward<Ts>(params)...));
    }
private:
    void init_offset(primary_key const *);
    const std::string m_name; 
    const columns m_schema;
    std::vector<size_t> m_offset; // fixed columns offset
};

inline bool usertable::column::is_fixed(syscolpars_row const * p, sysscalartypes_row const * s){
    SDL_ASSERT(p && s);    
    if (scalartype::is_fixed(s->data.id)) {
        if (!p->data.length.is_var()) {
            SDL_ASSERT(p->data.length._16 > 0);
            return true;
        }
    }
    return false;
}

inline bool usertable::column::is_fixed() const {
    return column::is_fixed(colpar, scalar);
}

inline bool usertable::column::is_array() const {
    switch (this->type) {
    case scalartype::t_char:
    case scalartype::t_nchar:
        SDL_ASSERT(length._16 > 0);
        return length._16 > 0;
    default:
        break;
    }
    return false;
}

using shared_usertable = std::shared_ptr<usertable>;
using vector_shared_usertable = std::vector<shared_usertable>;

} // db
} // sdl

#endif // __SDL_SYSTEM_USERTABLE_H__
