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
    using colpars = std::vector<syscolpars_row const *>;
    using scalars = std::vector<sysscalartypes_row const *>;
    using orders = std::vector<sortorder>;
public:
    page_head const * const root;
    const colpars colpar;
    const scalars scalar;
    const orders order;
public:
    primary_key(page_head const *, colpars &&, scalars &&, orders &&);
    size_t size() const {
        return colpar.size();
    }
    using colpar_scalar = std::pair<syscolpars_row const *, sysscalartypes_row const *>;
    colpar_scalar primary() const {
        return { colpar[0], scalar[0] };
    }
    sortorder first_order() const {
        return order[0];
    }
    bool is_index() const {
        return root->is_index();
    }
    colpars::const_iterator find_colpar(syscolpars_row const * p) const {
        return std::find(colpar.begin(), colpar.end(), p);
    }
};

using shared_primary_key = std::shared_ptr<primary_key>;

class cluster_index: noncopyable {
public:
    using shared_usertable = std::shared_ptr<usertable>;
    using column = usertable::column;
    using column_ref = column const &;
    using column_index = std::vector<size_t>; 
    using column_order = std::vector<sortorder>;
public:
    page_head const * const root;
    cluster_index(page_head const *, column_index &&, column_order &&, shared_usertable const &);
    size_t key_length() const {
        return m_key_length;
    }
    size_t sub_key_length(size_t i) const {
        SDL_ASSERT(i < col_index.size());
        return m_sub_key_length[i];
    }
    size_t size() const {
        return col_index.size();
    }
    column_ref operator[](size_t) const;
    sortorder order(size_t) const;

    template<class fun_type>
    void for_column(fun_type fun) const {
        for (size_t i = 0; i < size(); ++i) {
            fun((*this)[i]);
        }
    }
private:
    void init_key_length();
    column_index const col_index;
    column_order const col_ord;
    shared_usertable const schema;
    size_t m_key_length = 0;                // key memory size
    std::vector<size_t> m_sub_key_length;   // sub-key memory size
};

using unique_cluster_index = std::unique_ptr<cluster_index>;

} // db
} // sdl

#endif // __SDL_SYSTEM_USERTABLE_H__
