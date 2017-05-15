// maketable.h
//
#pragma once
#ifndef __SDL_SYSTEM_MAKETABLE_H__
#define __SDL_SYSTEM_MAKETABLE_H__

#include "dataserver/maketable/maketable_base.h"
#include "dataserver/maketable/maketable_where.h"
#include "dataserver/system/index_tree_t.h"
#include "dataserver/spatial/interval_set.h"
#include "dataserver/common/algorithm.h"

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

using pair_break_or_continue_bool = std::pair<break_or_continue, bool>;
inline pair_break_or_continue_bool make_break_or_continue_bool(bool b) {
    return { bc::continue_, b };
}
inline pair_break_or_continue_bool make_break_or_continue_bool(pair_break_or_continue_bool b) {
    return b;
}

namespace make_query_impl_ {
struct is_static_record_count_ {
private:
    template<typename T> static auto check(T const *) -> decltype(T::static_record_count);
    template<typename T> static void check(...);
public:
    template<typename T> 
    static auto test() -> decltype(check<T>(nullptr));
};
template<typename T> 
using is_static_record_count = identity<decltype(is_static_record_count_::test<T>())>;
} // make_query_impl_

template<class this_table, class RECORD_TYPE>
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
    using record = RECORD_TYPE;
    using record_range = std::vector<record>;
    using spatial_tree_T0 = typename clustered_traits::spatial_tree_T0;
    using spatial_page_row = typename clustered_traits::spatial_page_row;
    using pk0_type = T0_type;
private:
    this_table const & m_table;
    shared_cluster_index const m_cluster_index;
    using page_slot_bool = std::pair<page_slot, bool>;
public:
    make_query(this_table const * p, database const * const d)
        : m_table(*p)
        , m_cluster_index(d->get_cluster_index(_schobj_id(this_table::id)))
    {
        SDL_ASSERT(meta::test_clustered<table_clustered>());
        SDL_ASSERT((index_size != 0) == !!m_cluster_index);
        A_STATIC_CHECK_TYPE(schobj_id::type const, this_table::id);
    }
    template<class fun_type>
    void scan_if(fun_type && fun) const {
        for (record const & p : m_table) {
            if (!fun(p)) {
                break;
            }
        }
    }
    template<class fun_type>
    record find(fun_type && fun) const {
        for (record const & p : m_table) { // linear search
            if (fun(p)) {
                return p;
            }
        }
        return {};
    }
    template<typename... Ts>
    record find_with_index_n(Ts&&... params) const {
        static_assert(index_size == sizeof...(params), ""); 
        return find_with_index(make_key(std::forward<Ts>(params)...));
    }
    record find_with_index(key_type const &) const;
    page_slot_bool lower_bound(T0_type const &) const;

    static constexpr bool is_cluster_root_index() {
        return table_clustered::root_page_type == pageType::type::index;
    }
    static constexpr bool is_cluster_root_data() {
        return table_clustered::root_page_type == pageType::type::data;
    }
    template<class fun_type> page_slot scan_next(page_slot const &, fun_type &&) const;
    template<class fun_type> page_slot scan_prev(page_slot const &, fun_type &&) const;

    unique_spatial_tree_t<T0_type> get_spatial_tree() const {
        A_STATIC_ASSERT_NOT_TYPE(NullType, T0_type);
        return m_table.get_table().get_spatial_tree(identity<T0_type>());
    }
private:
    size_t record_count(identity<void>) const { 
        return m_table.get_table()._record.count(); // can be slow
    }
    static constexpr size_t record_count(identity<size_t>) { 
        return this_table::static_record_count;
    }
    record find_with_index(key_type const &, pageType_t<pageType::type::index>) const;
    record find_with_index(key_type const &, pageType_t<pageType::type::data>) const;

    page_slot_bool lower_bound(T0_type const &, pageType_t<pageType::type::index>) const;
    page_slot_bool lower_bound(T0_type const &, pageType_t<pageType::type::data>) const;
public:
    size_t record_count() const {
        return record_count(make_query_impl_::is_static_record_count<this_table>());
    }
    static constexpr size_t static_record_count() {
        return this_table::static_record_count;
    }
public:
    class seek_table; friend seek_table;
    class seek_spatial; friend seek_spatial;
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
    static bool equal_key(record const & src, key_type const & key) {
        key_type dest; // uninitialized
        make_query::read_key(dest, src);
        return dest == key;
    }
    static bool less_key(record const & src, key_type const & key) {
        key_type dest; // uninitialized
        make_query::read_key(dest, src);
        return dest < key;
    }
    template<typename... Ts> static
    key_type make_key(Ts&&... params) {
        static_assert(index_size == sizeof...(params), "make_key"); 
        key_type dest; // uninitialized
        set_key<0>(dest, params...);
        return dest;
    }
public:
    static bool push_unique(record_range &, record const &);
    
    template<class fun_type>
    static pair_break_or_continue_bool push_unique(fun_type && fun, record const & p) { // used with for_record
        return { make_break_or_continue(fun(p)), false };
    }
    static pair_break_or_continue_bool push_back(record_range & result, record const & p) {
        SDL_ASSERT(result.empty() || (read_key(result.back()) < read_key(p)));
        result.push_back(p);
        return { bc::continue_, true };
    }
    template<class fun_type>
    static pair_break_or_continue_bool push_back(fun_type && fun, record const & p) { // used with for_record
        return { make_break_or_continue(fun(p)), false };
    }
private:
    record get_record(row_head const * h) const {
        SDL_ASSERT(h->use_record());
        return record(&m_table, h);
    }
    record get_record(page_slot const & pos) const {
        return get_record(datapage(pos.page)[pos.slot]);
    }
    template<class col>
    typename col::ret_type col_value(row_head const * h) const {
        return get_record(h).val(identity<col>{});
    }
public: // for index_tree<KEY_TYPE>::first_page
    bool equal_first_key(row_head const * h, T0_type const & value) const {
        return meta::is_equal<T0_col>::equal(value, col_value<T0_col>(h)); 
    }
private:
    template<class col>
    bool key_less(row_head const * h, typename col::val_type const & value) const {
        return meta::key_less<col>::less(col_value<col>(h), value);
    }
    template<class col>
    bool key_less(typename col::val_type const & value, row_head const * h) const {
        return meta::key_less<col>::less(value, col_value<col>(h));
    }
    std::pair<page_slot, bool> lower_bound(page_head const *, T0_type const & value) const;
private:
    using select_expr = select_::select_expr<make_query>;
public:
    template<class sub_expr_type>
    record_range VALUES(sub_expr_type const & expr);

    template<class sub_expr_type>
    size_t COUNT(sub_expr_type const & expr);

    template<class sub_expr_type, class fun_type>
    void for_record(sub_expr_type const &, fun_type &&);
public:
    select_expr SELECT { this };
};

} // make
} // db
} // sdl

#include "dataserver/maketable/maketable_scan.hpp"
#include "dataserver/maketable/maketable_select.hpp"

#endif // __SDL_SYSTEM_MAKETABLE_H__
