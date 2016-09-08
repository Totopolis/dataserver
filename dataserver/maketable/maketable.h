// maketable.h
//
#pragma once
#ifndef __SDL_SYSTEM_MAKETABLE_H__
#define __SDL_SYSTEM_MAKETABLE_H__

#include "maketable_base.h"
#include "maketable_where.h"
#include "system/index_tree_t.h"

namespace sdl { namespace db { namespace make {

using where_::condition;
using where_::condition_t;

struct page_slot {
    page_head const * page = nullptr;
    size_t slot = 0;
    page_slot() = default;
    page_slot(page_head const * p, size_t s): page(p), slot(s) {
        SDL_ASSERT(page && page->data.pageId);
    }
};

template<class this_table, class _record>
class make_query: noncopyable {
    using table_clustered = typename this_table::clustered;
    using clustered_traits = meta::clustered_traits<table_clustered>;
    using KEY_TYPE = typename clustered_traits::key_type;
    using KEY_TYPE_LIST = typename clustered_traits::type_list;
    using T0_col = typename clustered_traits::T0_col;
    using T0_type = typename clustered_traits::T0_type;
    enum { index_size = clustered_traits::index_size };
public:
    using key_type = KEY_TYPE;
    using record = _record;
    using record_range = std::vector<record>;   //FIXME: optimize
private:
    this_table & m_table;
    page_head const * const m_cluster;
public:
    make_query(this_table * p, database * const d)
        : m_table(*p)
        , m_cluster(d->get_cluster_root(_schobj_id(this_table::id)))
    {
        SDL_ASSERT(meta::test_clustered<table_clustered>());
        SDL_ASSERT((index_size != 0) == (m_cluster != nullptr));
    }
    template<class fun_type>
    void scan_if(fun_type fun) const {
        for (auto p : m_table) {
            A_STATIC_CHECK_TYPE(record, p);
            if (!fun(p)) {
                break;
            }
        }
    }
    template<class fun_type>
    record find(fun_type fun) const {
        for (auto p : m_table) { // linear search
            A_STATIC_CHECK_TYPE(record, p);
            if (fun(p)) {
                return p;
            }
        }
        return {};
    }
    template<typename... Ts>
    record find_with_index_n(Ts&&... params) const {
        static_assert(index_size == sizeof...(params), ""); 
        return find_with_index(make_key(params...));
    }
    record find_with_index(key_type const &) const;
    std::pair<page_slot, bool> lower_bound(T0_type const &) const;

    template<class fun_type> page_slot scan_next(page_slot const &, fun_type) const;
    template<class fun_type> page_slot scan_prev(page_slot const &, fun_type) const;
#if 1
    shared_spatial_tree_t<T0_type> get_spatial_tree() const {
        A_STATIC_ASSERT_NOT_TYPE(NullType, T0_type);
        return m_table.get_table().get_spatial_tree(identity<T0_type>());
    }
#else
    auto get_spatial_tree() const { //FIXME: deduced return types are C++14 extension
        A_STATIC_ASSERT_NOT_TYPE(NullType, T0_type);
        return m_table.get_table().get_spatial_tree(identity<T0_type>());
    }
#endif
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
private:
    record get_record(row_head const * h) const {
        SDL_ASSERT(h->use_record()); //FIXME: check possibility
        return record(&m_table, h);
    }
    record get_record(page_slot const & pos) const {
        return get_record(datapage(pos.page)[pos.slot]);
    }
    template<class col>
    typename col::ret_type col_value(row_head const * h) const {
        return get_record(h).val(identity<col>{});
    }
    template<class col>
    bool key_less(row_head const * h, typename col::val_type const & value) const {
        return meta::key_less<col>::less(col_value<col>(h), value);
    }
    template<class col>
    bool key_less(typename col::val_type const & value, row_head const * h) const {
        return meta::key_less<col>::less(value, col_value<col>(h));
    }
private:
    using select_expr = select_::select_expr<make_query>;
public:
    template<class sub_expr_type>
    record_range VALUES(sub_expr_type const & expr);
public:
    select_expr SELECT { this };
    //FIXME: select * from GeoTable as gt where myPoint.STDistance(gt.Geo) <= 50
    //FIXME: STDistance, STContains
};

} // make
} // db
} // sdl

#include "maketable_scan.hpp"
#include "maketable_select.hpp"

#endif // __SDL_SYSTEM_MAKETABLE_H__
