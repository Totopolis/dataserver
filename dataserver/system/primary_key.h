// primary_key.h
//
#pragma once
#ifndef __SDL_SYSTEM_PRIMARY_KEY_H__
#define __SDL_SYSTEM_PRIMARY_KEY_H__

#include "usertable.h"

namespace sdl { namespace db {

class primary_key: noncopyable {
public:
    using colpars = std::vector<syscolpars_row const *>;  //FIXME: vector_buf<T, 2> ?
    using scalars = std::vector<sysscalartypes_row const *>;
    using orders = std::vector<sortorder>;
public:
    page_head const * const root;
    sysidxstats_row const * const idxstat;
    const colpars colpar;
    const scalars scalar;
    const orders order;
    schobj_id const table_id;
public:
    primary_key(page_head const *, sysidxstats_row const *,
        colpars &&, scalars &&, orders &&, schobj_id);
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
    bool is_data() const {
        return root->is_data();
    }
    colpars::const_iterator find_colpar(syscolpars_row const * p) const {
        return std::find(colpar.begin(), colpar.end(), p);
    }
    std::string name() const {
        return col_name_t(idxstat);
    }
};

using shared_primary_key = std::shared_ptr<primary_key>;

class cluster_index: noncopyable {
public:
    using column = usertable::column;
    using column_ref = column const &;
    using column_index = std::vector<size_t>;       //FIXME: vector_buf<T, 2> ?
    using column_order = std::vector<sortorder>;    //FIXME: vector_buf<T, 2> ?
public:
    cluster_index(shared_primary_key const &, shared_usertable const &, column_index &&);
    
    page_head const * root() const {
        return primary->root;
    }
    bool is_root_index() const {
        return root()->is_index();
    }
    bool is_root_data() const {
        return root()->is_data();
    }
    schobj_id get_id() const {
        return primary->table_id;
    }
    std::string name() const {
        return primary->name();
    }
    size_t size() const {
        return m_index.size();
    }
    size_t col_ind(size_t i) const {
        SDL_ASSERT(i < size());
        return m_index[i];
    }
    sortorder col_ord(size_t i) const {
        SDL_ASSERT(i < size());
        return primary->order[i];
    }
    size_t key_length() const {
        return m_key_length;
    }
    size_t sub_key_length(size_t i) const {
        SDL_ASSERT(i < size());
        return m_sub_key_length[i];
    }
    column_ref operator[](size_t i) const {
       SDL_ASSERT(i < size());
        return (*m_schema)[m_index[i]];
    }
    column_ref get_column(size_t i) const {
        return (*this)[i];
    }
    sortorder order(size_t i) const {
        SDL_ASSERT(i < size());
        return primary->order[i];
    }
    bool is_descending(size_t i) const {
        SDL_ASSERT(i < size());
        return (sortorder::DESC == col_ord(i));
    }
    template<class fun_type>
    void for_column(fun_type && fun) const {
        for (size_t i = 0, end = size(); i < end; ++i) {
            fun((*this)[i]);
        }
    }
private:
    shared_primary_key const primary;
    shared_usertable const m_schema;
    column_index const m_index;
    size_t m_key_length = 0;                // key memory size
    std::vector<size_t> m_sub_key_length;   // sub-key memory size
};

using unique_cluster_index = std::unique_ptr<cluster_index>;
using shared_cluster_index = std::shared_ptr<cluster_index>;

} // db
} // sdl

#endif // __SDL_SYSTEM_PRIMARY_KEY_H__
