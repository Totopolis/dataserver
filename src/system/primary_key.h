// primary_key.h
//
#ifndef __SDL_SYSTEM_PRIMARY_KEY_H__
#define __SDL_SYSTEM_PRIMARY_KEY_H__

#pragma once

#include "usertable.h"

namespace sdl { namespace db {

class primary_key: noncopyable {
public:
    using colpars = std::vector<syscolpars_row const *>;
    using scalars = std::vector<sysscalartypes_row const *>;
    using orders = std::vector<sortorder>;
public:
    page_head const * const root;
    sysidxstats_row const * const idxstat;
    const colpars colpar;
    const scalars scalar;
    const orders order;
public:
    primary_key(page_head const *, sysidxstats_row const *,
        colpars &&, scalars &&, orders &&);
    size_t size() const {
        return colpar.size();
    }
    syscolpars_row const * primary() const {
        return colpar[0];
    }
    sysscalartypes_row const * first_scalar() const {
        return scalar[0];
    }
    scalartype::type first_type() const {
        return first_scalar()->data.id;
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
    using column = usertable::column;
    using column_ref = column const &;
    using column_index = std::vector<size_t>; 
    using column_order = std::vector<sortorder>;
public:
    page_head const * const root;
    column_index const col_index;
    column_order const col_ord;
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
    column_ref operator[](size_t i) const {
       SDL_ASSERT(i < size());
        return (*schema)[col_index[i]];
    }
    sortorder order(size_t i) const {
        SDL_ASSERT(i < size());
        return col_ord[i];
    }
    bool is_descending(size_t i) const {
        SDL_ASSERT(i < size());
        return (sortorder::DESC == col_ord[i]);
    }
    template<class fun_type>
    void for_column(fun_type fun) const {
        for (size_t i = 0; i < size(); ++i) {
            fun((*this)[i]);
        }
    }
private:
    shared_usertable const schema;
    size_t m_key_length = 0;                // key memory size
    std::vector<size_t> m_sub_key_length;   // sub-key memory size
};

using unique_cluster_index = std::unique_ptr<cluster_index>;

} // db
} // sdl

#endif // __SDL_SYSTEM_PRIMARY_KEY_H__
