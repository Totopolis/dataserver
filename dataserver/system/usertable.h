// usertable.h
//
#pragma once
#ifndef __SDL_SYSTEM_USERTABLE_H__
#define __SDL_SYSTEM_USERTABLE_H__

#include "dataserver/system/datapage.h"
#include "dataserver/system/scalartype_t.h"

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
        bool is_geography() const {
            return scalartype::t_geography == this->type;
        }
        size_t fixed_size() const {
            SDL_ASSERT(is_fixed());
            return length._16;
        }
        std::string type_schema(primary_key const * PK = nullptr) const;
    };
    using unique_col = std::unique_ptr<column>;
    using column_ref = column const &;
    using columns = std::vector<unique_col>;
    using iterator = forward_iterator<usertable const, columns::const_iterator>;
private:
    friend iterator;
    static void load_next(columns::const_iterator & it) {
        ++it;
    }
    static column_ref dereference(columns::const_iterator const & it) {
        return *(*it);
    }
public:
    sysschobjs_row const * const schobj;
    usertable(sysschobjs_row const *, columns &&, primary_key const *);

    schobj_id get_id() const {
        return schobj->data.id;
    }
    nsid_id get_nsid() const {
        return schobj->data.nsid;
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
    iterator begin() const {
        return iterator(this, m_schema.begin());
    }
    iterator end() const {
        return iterator(this, m_schema.end());
    }
    std::string type_schema(primary_key const * PK = nullptr) const;

    template<class fun_type>
    void for_col(fun_type && fun) const {
        for (auto const & c : m_schema) {
            fun(*c);
        }
    }
    template<class fun_type>
    size_t count_if(fun_type && fun) const {
        size_t ret = 0;
        for (auto const & c : m_schema) {
            if (fun(*c))
                ++ret;
        }
        return ret;
    }
    template<class fun_type>
    size_t find_if(fun_type && fun) const {
        size_t ret = 0;
        for (auto const & c : m_schema) {
            if (fun(*c))
                break;
            ++ret;
        }
        return ret; // return size() if column not found
    }
    size_t find(const std::string & name) const {
        return find_if([&name](column_ref c){
            return c.name == name;
        });
    }
    size_t find(const char * const name) const {
        SDL_ASSERT(is_str_valid(name));
        return find_if([name](column_ref c){
            return c.name == name;
        });
    }
    size_t find_geography() const;
    bool is_geography() const {
        return find_geography() < schema().size();
    }
    using col_index = std::pair<column const *, size_t>;
    col_index find_col(syscolpars_row const *) const;

    bool is_fixed(size_t i) const {
        return (*this)[i].is_fixed();
    }
    size_t count_var() const;       // # of variable cols
    size_t count_fixed() const;     // # of fixed cols
    size_t fixed_size() const;
    size_t fixed_offset(size_t) const;
    size_t var_offset(size_t) const;
    size_t offset(size_t i) const { return m_offset[i]; }
    size_t place(size_t) const;

    template<typename... Ts> static
    void emplace_back(columns & cols, Ts&&... params) {
        cols.push_back(std::make_unique<column>(std::forward<Ts>(params)...));
    }
private:
    void init_offset(primary_key const *);
    const std::string m_name; 
    const columns m_schema;
    std::vector<size_t> m_offset;  // fixed columns offset
    std::vector<size_t> m_place;   // columns memory order
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

inline size_t usertable::fixed_offset(size_t const i) const {
    SDL_ASSERT(i < size());
    SDL_ASSERT(m_schema[i]->is_fixed());
    return m_offset[i];
}

inline size_t usertable::var_offset(size_t const i) const {
    SDL_ASSERT(i < size());
    SDL_ASSERT(!m_schema[i]->is_fixed());
    return m_offset[i];
}

inline size_t usertable::place(size_t const i) const {
    SDL_ASSERT(i < size());
    return m_place[i];
}

using shared_usertable = std::shared_ptr<usertable>;
using vector_shared_usertable = std::vector<shared_usertable>;

} // db
} // sdl

#endif // __SDL_SYSTEM_USERTABLE_H__
