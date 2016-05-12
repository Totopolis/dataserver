// maketable.h
//
#pragma once
#ifndef __SDL_SYSTEM_MAKETABLE_H__
#define __SDL_SYSTEM_MAKETABLE_H__

#include "maketable_base.h"
#include "maketable_where.h"
#include "index_tree_t.h"

namespace sdl { namespace db { namespace make {

using where_::condition;
using where_::condition_t;

template<class this_table, class _record>
class make_query: noncopyable {
    using table_clustered = typename this_table::clustered;
    using KEY_TYPE = meta::cluster_key<table_clustered, NullType>;
    using KEY_TYPE_LIST = meta::cluster_type_list<table_clustered, NullType>;
    enum { index_size = meta::cluster_index_size<table_clustered>::value };
public:
    using key_type = KEY_TYPE;
    using first_key = meta::cluster_first_key<table_clustered, NullType>;
    using record = _record;
    using record_range = std::vector<record>;
private:
    this_table & m_table;
    page_head const * const m_cluster;
public:
    make_query(this_table * p, database * const d, shared_usertable const & s)
        : m_table(*p)
        , m_cluster(d->get_cluster_root(_schobj_id(this_table::id)))
    {
        SDL_ASSERT(meta::test_clustered<table_clustered>());
    }
    template<class fun_type>
    void scan_if(fun_type fun) {
        for (auto p : m_table) {
            A_STATIC_CHECK_TYPE(record, p);
            if (!fun(p)) {
                break;
            }
        }
    }
    template<class fun_type>
    record find(fun_type fun) {
        for (auto p : m_table) { // linear search
            A_STATIC_CHECK_TYPE(record, p);
            if (fun(p)) {
                return p;
            }
        }
        return {};
    }
    template<typename... Ts>
    record find_with_index_n(Ts&&... params) {
        static_assert(index_size == sizeof...(params), ""); 
        return find_with_index(make_key(params...));
    }
    record find_with_index(key_type const &);

private:
    template<class value_type, class fun_type,
        class is_equal_type = meta::is_equal<first_key>>
    break_or_continue scan_with_index(value_type const & value, fun_type, is_equal_type is_equal = is_equal_type());
private:
    template<typename... Ts>
    record find_ignore_index_n(Ts&&... params) {
        static_assert(index_size == sizeof...(params), ""); 
        return find_ignore_index(make_key(params...));
    }
    record find_ignore_index(key_type const & key) {
        for (auto p : m_table) { // linear search
            A_STATIC_CHECK_TYPE(record, p);
            if (key == read_key(p)) {
                return p;
            }
        }
        return {};
    }
public:
    class seek_table;
    friend seek_table;
private:
    template<class T> // T = meta::index_col
    using key_index = TL::IndexOf<KEY_TYPE_LIST, T>;

    template<size_t i>
    using key_index_at = typename TL::TypeAt<KEY_TYPE_LIST, i>::Result;

    class read_key_fun {
        key_type & dest;
        record const & src;
    public:
        read_key_fun(key_type & d, record const & s) : dest(d), src(s) {}
        template<class T> // T = meta::index_col
        bool operator()(identity<T>) const {
            enum { set_index = key_index<T>::value };
            A_STATIC_ASSERT_IS_POD(typename T::type);
            meta::copy(dest.set(Int2Type<set_index>()), src.val(identity<typename T::col>()));
            return true;
        }
    };
    template<size_t i> static void set_key(key_type &) {}
    template<size_t i, typename T, typename... Ts>
    static void set_key(key_type & dest, T && value, Ts&&... params) {
        dest.set(Int2Type<i>()) = std::forward<T>(value);
        set_key<i+1>(dest, params...);
    }
public:
    static void read_key(key_type & dest, record const & src) {
        meta::processor_if<KEY_TYPE_LIST>::apply(read_key_fun(dest, src));
    }
    static key_type read_key(record const & src) {
        key_type dest; // uninitialized
        make_query::read_key(dest, src);
        return dest;
    }
    key_type read_key(row_head const * h) const {
        SDL_ASSERT(h);
        return make_query::read_key(record(&m_table, h));
    }
    template<typename... Ts> static
    key_type make_key(Ts&&... params) {
        static_assert(index_size == sizeof...(params), "make_key"); 
        key_type dest; // uninitialized
        set_key<0>(dest, params...);
        return dest;
    }
    record get_record(row_head const * h) const {
        return record(&m_table, h);
    }
private:
    //FIXME: select * from GeoTable as gt where myPoint.STDistance(gt.Geo) <= 50
    using select_expr = select_::select_expr<make_query>;
public:
    template<class sub_expr_type>
    record_range VALUES(sub_expr_type const & expr);
public:
    select_expr SELECT { this };
};

} // make
} // db
} // sdl

#include "maketable_scan.hpp"
#include "maketable_select.hpp"

#endif // __SDL_SYSTEM_MAKETABLE_H__
