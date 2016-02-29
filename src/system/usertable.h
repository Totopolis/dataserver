// usertable.h
//
#ifndef __SDL_SYSTEM_USERTABLE_H__
#define __SDL_SYSTEM_USERTABLE_H__

#pragma once

#include "datapage.h"

namespace sdl { namespace db {

class primary_key;

class usertable : noncopyable {
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

        bool is_fixed() const;

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

class primary_key: noncopyable {
public:
    using columns = std::vector<syscolpars_row const *>;
    page_head const * const root;
    const columns cols;
    primary_key(page_head const * p, columns && c)
        : root(p), cols(std::move(c))
    {
        SDL_ASSERT(root);
        SDL_ASSERT(!cols.empty());
    }
    syscolpars_row const * primary() const {
        return cols[0];
    }
};

using shared_primary_key = std::shared_ptr<primary_key>;

class cluster_index: noncopyable {
public:
    using shared_usertable = std::shared_ptr<usertable>;
    using column = usertable::column;
    using column_index = std::vector<size_t>; 
    using column_ref = column const &;
public:
    page_head const * const root;
    cluster_index(page_head const * p, column_index && c, shared_usertable const & sch)
        : root(p), col_index(std::move(c)), schema(sch)
    {
        SDL_ASSERT(root);
        SDL_ASSERT(!col_index.empty());
        SDL_ASSERT(schema.get());
    }
    size_t key_length() const; // key memory size
    size_t sub_key_length(size_t) const; // sub-key memory size

    size_t size() const {
        return col_index.size();
    }
    column_ref operator[](size_t i) const {
        SDL_ASSERT(i < col_index.size());
        return (*schema)[col_index[i]];
    }
    template<class fun_type>
    void for_column(fun_type fun) const {
        for (size_t i = 0; i < size(); ++i) {
            fun((*this)[i]);
        }
    }
private:
    column_index const col_index;
    shared_usertable const schema;
};

using unique_cluster_index = std::unique_ptr<cluster_index>;

template<typename T, scalartype::type type> inline
T const * scalartype_cast(mem_range_t const & m, usertable::column const & col) {
    if (col.type == type) {
        if ((mem_size(m) == sizeof(T)) && (col.fixed_size() == sizeof(T))) {
            return reinterpret_cast<const T *>(m.first);
        }
        SDL_ASSERT(!"scalartype_cast");
    }
    return nullptr; 
}

} // db
} // sdl

#endif // __SDL_SYSTEM_USERTABLE_H__
