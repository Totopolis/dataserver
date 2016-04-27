// maketable.hpp
//
#pragma once
#ifndef __SDL_SYSTEM_MAKETABLE_HPP__
#define __SDL_SYSTEM_MAKETABLE_HPP__

namespace sdl { namespace db { namespace make {

template<class this_table, class record>
record make_query<this_table, record>::find_with_index(key_type const & key) {
    if (m_cluster) {
        auto const db = m_table.get_db();
        if (auto const id = make::index_tree<key_type>(db, m_cluster).find_page(key)) {
            if (page_head const * const h = db->load_page_head(id)) {
                SDL_ASSERT(h->is_data());
                const datapage data(h);
                if (!data.empty()) {
                    size_t const slot = data.lower_bound(
                        [this, &key](row_head const * const row, size_t) {
                        return (this->read_key(row) < key);
                    });
                    if (slot < data.size()) {
                        row_head const * const head = data[slot];
                        if (!(key < read_key(head))) {
                            return record(&m_table, head);
                        }
                    }
                }
            }
        }
    }
    return {};
}

#if maketable_select_old_code
template<class this_table, class record>
typename make_query<this_table, record>::record_range
make_query<this_table, record>::select(select_key_list in, ignore_index, unique_false) {
    record_range result;
    if (in.size()) {
        result.reserve(in.size());
        for (auto p : m_table) { // scan all table
            A_STATIC_CHECK_TYPE(record, p);
            auto const key = make_query::read_key(p);
            for (auto const & k : in) {
                if (key == k) {
                    result.push_back(p);
                }
            }
        }
    }
    return result;              
}

template<class this_table, class record>
typename make_query<this_table, record>::record_range
make_query<this_table, record>::select(select_key_list in, ignore_index, unique_true) {
    record_range result;
    if (in.size()) {
        result.reserve(in.size());
        std::vector<const key_type *> look(in.size());
        for (size_t i = 0; i < in.size(); ++i) {
            look[i] = in.begin() + i;
        }
        size_t size = in.size();
        for (auto p : m_table) { // scan table and filter found keys
            A_STATIC_CHECK_TYPE(record, p);
            auto const key = make_query::read_key(p);
            for (size_t i = 0; i < size; ++i) {
                if (key == *look[i]) {
                    result.push_back(p);
                    if (!(--size))
                        return result;
                    look[i] = look[size];
                }
            }
        }
    }
    return result; // not all keys found
}

template<class this_table, class record>
typename make_query<this_table, record>::record_range
make_query<this_table, record>::select(select_key_list in, use_index, unique_true) {
    record_range result;
    if (in.size()) {
        result.reserve(in.size());
        for (auto const & key : in) {
            if (auto p = find_with_index(key)) {
                result.push_back(p);
            }
        }
    }
    return result;
}

/*template<class this_table, class record>
typename make_query<this_table, record>::record_range
make_query<this_table, record>::select(select_key_list in, enum_index const v1, enum_unique const v2) {
    if (enum_index::ignore_index == v1) {
        if (enum_unique::unique_false == v2) {
            return select(in, enum_index_t<ignore_index>(), enum_unique_t<unique_false>());
        }
        SDL_ASSERT(enum_unique::unique_true == v2);
        return select(in, enum_index_t<ignore_index>(), enum_unique_t<unique_true>());
    }
    SDL_ASSERT(enum_index::use_index == v1);
    SDL_ASSERT(enum_unique::unique_true == v2);
    return select(in, enum_index_t<use_index>(), enum_unique_t<unique_true>());
}*/

/*template<typename T, typename... Ts> 
void select_n(where<T> col, Ts const & ... params) {
    enum { col_found = TL::IndexOf<typename this_table::type_list, T>::value };
    enum { key_found = meta::cluster_col_find<KEY_TYPE_LIST, T>::value };
    static_assert(col_found != -1, "");
    using type_list = typename TL::Seq<T, Ts...>::Type; // test
    static_assert(TL::Length<type_list>::value == sizeof...(params) + 1, "");
    //SDL_TRACE(typeid(type_list).name());
    SDL_ASSERT(where<T>::name() == T::name()); // same memory
    SDL_TRACE(
        "col:", col_found, 
        " key:", key_found, 
        " name:", T::name(),
        " value:", col.value);
    select_n(params...);
}*/
#endif

namespace make_query_ {

using where_::condition;
using where_::condition_t;

//--------------------------------------------------------------

template<class T, where_::condition cond = T::cond>
struct use_index {
    static_assert((T::hint != where_::INDEX::USE) || T::col::PK, "INDEX::USE");
    enum { value = T::col::PK && (T::hint != where_::INDEX::IGNORE) };
};

template<class T>
struct use_index<T, where_::condition::_lambda> {
    enum { value = false };
};

template<class T>
struct use_index<T, where_::condition::_order> {
    enum { value = false };
};

//--------------------------------------------------------------

template<class T, where_::condition cond = T::cond>
struct ignore_index {
    static_assert((T::hint != where_::INDEX::USE) || T::col::PK, "INDEX::USE");
    enum { value = !T::col::PK || (T::hint == where_::INDEX::IGNORE) };
};

template<class T>
struct ignore_index<T, where_::condition::_lambda> {
    enum { value = true };
};

template<class T>
struct ignore_index<T, where_::condition::_order> {
    enum { value = true };
};

//--------------------------------------------------------------

template <class TList, class OList, size_t> struct search_use_index;
template <size_t i> struct search_use_index<NullType, NullType, i>
{
    using Index = NullType;
    using Types = NullType;
    using OList = NullType;
};

template <class Head, class Tail, where_::operator_ OP, class NextOP, size_t i>
struct search_use_index<Typelist<Head, Tail>, where_::operator_list<OP, NextOP>, i> {
private:
    enum { flag = use_index<Head>::value };
    using indx_i = typename Select<flag, Typelist<Int2Type<i>, NullType>, NullType>::Result;
    using type_i = typename Select<flag, Typelist<Head, NullType>, NullType>::Result;
    using oper_i = typename Select<flag, Typelist<where_::operator_t<OP>, NullType>, NullType>::Result;
public:
    using Types = typename TL::Append<type_i, typename search_use_index<Tail, NextOP, i + 1>::Types>::Result; 
    using Index = typename TL::Append<indx_i, typename search_use_index<Tail, NextOP, i + 1>::Index>::Result;
    using OList = typename TL::Append<oper_i, typename search_use_index<Tail, NextOP, i + 1>::OList>::Result;
};
//--------------------------------------------------------------

template <class TList, class OList, size_t> struct search_ignore_index;
template <size_t i> struct search_ignore_index<NullType, NullType, i>
{
    using Index = NullType;
    using Types = NullType;
    using OList = NullType;
};

template <class Head, class Tail, where_::operator_ OP, class NextOP, size_t i>
struct search_ignore_index<Typelist<Head, Tail>, where_::operator_list<OP, NextOP>, i> {
private:
    enum { flag = ignore_index<Head>::value };
    using indx_i = typename Select<flag, Typelist<Int2Type<i>, NullType>, NullType>::Result;
    using type_i = typename Select<flag, Typelist<Head, NullType>, NullType>::Result;
    using oper_i = typename Select<flag, Typelist<where_::operator_t<OP>, NullType>, NullType>::Result;
public:
    using Types = typename TL::Append<type_i, typename search_ignore_index<Tail, NextOP, i + 1>::Types>::Result; 
    using Index = typename TL::Append<indx_i, typename search_ignore_index<Tail, NextOP, i + 1>::Index>::Result;
    using OList = typename TL::Append<oper_i, typename search_ignore_index<Tail, NextOP, i + 1>::OList>::Result;
};

//--------------------------------------------------------------

template<class TList> struct process_push_back;
template<> struct process_push_back<NullType>
{
    template<class T>
    static void push_back(T &){}
};

template<size_t i, class Tail> 
struct process_push_back<Typelist<Int2Type<i>, Tail>>
{
    template<class T>    
    static void push_back(T & dest){
        A_STATIC_ASSERT_TYPE(size_t, typename T::value_type);
        dest.push_back(i);
        process_push_back<Tail>::push_back(dest);
    }
};

template<class TList, class T> 
inline void push_back(T & dest) {
    dest.reserve(TL::Length<TList>::value);
    process_push_back<TList>::push_back(dest);
}

//--------------------------------------------------------------

template<size_t i, class T, where_::operator_ op> // T = where_::SEARCH
struct search_where
{
    enum { offset = i };
    using type = T;
    using col = typename T::col;
    static const where_::operator_ OP = op;
};

template<class Index, class TList, class OList> struct make_search_where;
template<> struct make_search_where<NullType, NullType, NullType>
{
    using Result = NullType;
};

template<
    size_t i, class NextIndex, 
    class T, class NextType,
    where_::operator_ OP,  class NextOP>
struct make_search_where<
    Typelist<Int2Type<i>, NextIndex>,
    Typelist<T, NextType>,
    Typelist<where_::operator_t<OP>, NextOP>>
{
private:
    using Item = Typelist<search_where<i, T, OP>,  NullType>;
    using Next = typename make_search_where<NextIndex, NextType, NextOP>::Result;
public:
    using Result = typename TL::Append<Item, Next>::Result;
};

//--------------------------------------------------------------

template<class sub_expr_type>
struct SEARCH_USE_INDEX {
private:
    using use_index = search_use_index<
        typename sub_expr_type::type_list,
        typename sub_expr_type::oper_list,
        0>;
    using Index = typename use_index::Index;
    using Types = typename use_index::Types;
    using OList = typename use_index::OList;     
public:
    using Result = typename make_search_where<Index, Types, OList>::Result;
    static_assert(TL::Length<Index>::value == TL::Length<Types>::value, "");
    static_assert(TL::Length<Index>::value == TL::Length<OList>::value, "");
    static_assert(TL::Length<Result>::value == TL::Length<OList>::value, "");
};

template<class T>
using SEARCH_USE_INDEX_t = typename SEARCH_USE_INDEX<T>::Result;

//--------------------------------------------------------------

template<class sub_expr_type>
struct SEARCH_IGNORE_INDEX {
private:
    using ignore_index = search_ignore_index<
        typename sub_expr_type::type_list,
        typename sub_expr_type::oper_list,
        0>;
    using Index = typename ignore_index::Index;
    using Types = typename ignore_index::Types;
    using OList = typename ignore_index::OList;     
public:
    using Result = typename make_search_where<Index, Types, OList>::Result;
    static_assert(TL::Length<Index>::value == TL::Length<Types>::value, "");
    static_assert(TL::Length<Index>::value == TL::Length<OList>::value, "");
    static_assert(TL::Length<Result>::value == TL::Length<OList>::value, "");
};

template<class T>
using SEARCH_IGNORE_INDEX_t = typename SEARCH_IGNORE_INDEX<T>::Result;

//--------------------------------------------------------------

template <class _search_where>
struct SELECT_RECORD_WITH_INDEX {
private:
    template<class record_range, class query_type, class expr_type, condition cond> static
    void select_cond(record_range & result, query_type * const query, expr_type const * const expr, condition_t<cond>) {
        static_assert((cond == condition::WHERE) || (cond == condition::IN), "");
        for (auto const & v : expr->value.values) {
            if (auto record = query->find_with_index(query->make_key(v))) {
                result.push_back(record);
            }
        }
    }
    template<class record_range, class query_type, class expr_type> static
    void select_cond(record_range & result, query_type * const query, expr_type const * const expr, condition_t<condition::NOT>) {
    }
    template<class record_range, class query_type, class expr_type> static
    void select_cond(record_range & result, query_type * const query, expr_type const * const expr, condition_t<condition::LESS>) {
    }
    template<class record_range, class query_type, class expr_type> static
    void select_cond(record_range & result, query_type * const query, expr_type const * const expr, condition_t<condition::GREATER>) {
    }
    template<class record_range, class query_type, class expr_type> static
    void select_cond(record_range & result, query_type * const query, expr_type const * const expr, condition_t<condition::LESS_EQ>) {
    }
    template<class record_range, class query_type, class expr_type> static
    void select_cond(record_range & result, query_type * const query, expr_type const * const expr, condition_t<condition::GREATER_EQ>) {
    }
    template<class record_range, class query_type, class expr_type> static
    void select_cond(record_range & result, query_type * const query, expr_type const * const expr, condition_t<condition::BETWEEN>) {
    }
public:
    template<class record_range, class query_type, class sub_expr_type> static
    void select(record_range & result, query_type * query, sub_expr_type const & expr) {
        using T = _search_where;
        if (1) {
            SDL_TRACE("SELECT_RECORD_WITH_INDEX[", T::offset, "] = ",
                where_::operator_name<T::OP>(), " ",
                where_::condition_name<T::type::cond>()
                );
        }
        select_cond(result, query, expr.get(Size2Type<T::offset>()), condition_t<T::type::cond>());
    }
};

//--------------------------------------------------------------

template <class _search_where>
struct SELECT_RECORD_IGNORE_INDEX {
    using search_col = typename _search_where::col;
private:
    template<class query_type, class value_type> static
    typename query_type::record
    find(query_type * const query, value_type const & v, Int2Type<true>) {
        A_STATIC_ASSERT_NOT_TYPE(void, search_col);
        return query->find_ignore_index(query->make_key(v));
    }
    template<class query_type, class value_type> static
    typename query_type::record
    find(query_type * const query, value_type const & v, Int2Type<false>) {
        A_STATIC_ASSERT_NOT_TYPE(void, search_col);
        using search_val = typename search_col::val_type;
        return query->find([&v](typename query_type::record p) {
            return meta::is_equal<search_col>::equal(
                p.val(identity<search_col>{}), 
                static_cast<search_val const &>(v));
        });
    }
    template<class record_range, class query_type, class expr_type, condition cond> static
    void select_cond(record_range & result, query_type * const query, expr_type const * const expr, condition_t<cond>) {
        static_assert((cond == condition::WHERE) || (cond == condition::IN), "");
        for (auto const & v : expr->value.values) {
            if (auto record = find(query, v, Int2Type<_search_where::col::PK>{})) {
                result.push_back(record);
            }
        }
    }
    template<class record_range, class query_type, class expr_type> static
    void select_cond(record_range & result, query_type * const query, expr_type const * const expr, condition_t<condition::NOT>) {
    }
    template<class record_range, class query_type, class expr_type> static
    void select_cond(record_range & result, query_type * const query, expr_type const * const expr, condition_t<condition::LESS>) {
    }
    template<class record_range, class query_type, class expr_type> static
    void select_cond(record_range & result, query_type * const query, expr_type const * const expr, condition_t<condition::GREATER>) {
    }
    template<class record_range, class query_type, class expr_type> static
    void select_cond(record_range & result, query_type * const query, expr_type const * const expr, condition_t<condition::LESS_EQ>) {
    }
    template<class record_range, class query_type, class expr_type> static
    void select_cond(record_range & result, query_type * const query, expr_type const * const expr, condition_t<condition::GREATER_EQ>) {
    }
    template<class record_range, class query_type, class expr_type> static
    void select_cond(record_range & result, query_type * const query, expr_type const * const expr, condition_t<condition::BETWEEN>) {
    }
    template<class record_range, class query_type, class expr_type> static
    void select_cond(record_range & result, query_type * const query, expr_type const * const expr, condition_t<condition::_lambda>) {
    }
    template<class record_range, class query_type, class expr_type> static
    void select_cond(record_range & result, query_type * const query, expr_type const * const expr, condition_t<condition::_order>) {
    }
public:
    template<class record_range, class query_type, class sub_expr_type> static
    void select(record_range & result, query_type * query, sub_expr_type const & expr) {
        using T = _search_where;
        if (1) {
            SDL_TRACE("SELECT_RECORD_IGNORE_INDEX[", T::offset, "] = ",
                where_::operator_name<T::OP>(), " ",
                where_::condition_name<T::type::cond>()
                );
        }
        select_cond(result, query, expr.get(Size2Type<T::offset>()), condition_t<T::type::cond>());
    }
};
//--------------------------------------------------------------

template <class search_list> struct SELECT_WITH_INDEX;
template <> struct SELECT_WITH_INDEX<NullType>
{
    template<class record_range, class query_type, class sub_expr_type> static
    void select(record_range &, query_type *, sub_expr_type const &) {}
};

template<class T, class NextType> // T = search_key
struct SELECT_WITH_INDEX<Typelist<T, NextType>> {
public:
    template<class record_range, class query_type, class sub_expr_type> static
    void select(record_range & result, query_type * query, sub_expr_type const & expr) {
        SELECT_RECORD_WITH_INDEX<T>::select(result, query, expr);
        SELECT_WITH_INDEX<NextType>::select(result, query, expr);
    }
};

//--------------------------------------------------------------

template <class search_list> struct SELECT_IGNORE_INDEX;
template <> struct SELECT_IGNORE_INDEX<NullType>
{
    template<class record_range, class query_type, class sub_expr_type> static
    void select(record_range &, query_type *, sub_expr_type const &) {}
};

template<class T, class NextType> // T = search_key
struct SELECT_IGNORE_INDEX<Typelist<T, NextType>> {
public:
    template<class record_range, class query_type, class sub_expr_type> static
    void select(record_range & result, query_type * query, sub_expr_type const & expr) {
        SELECT_RECORD_IGNORE_INDEX<T>::select(result, query, expr);
        SELECT_IGNORE_INDEX<NextType>::select(result, query, expr);
    }
};

} // make_query_

//--------------------------------------------------------------

template<class this_table, class record>
template<class sub_expr_type>
typename make_query<this_table, record>::record_range
make_query<this_table, record>::VALUES(sub_expr_type const & expr)
{
    SDL_TRACE("\nVALUES:");
    if (1) {
        where_::trace_::trace_sub_expr(expr);
    }
    using use_index_t = make_query_::SEARCH_USE_INDEX_t<sub_expr_type>;
    using ignore_index_t = make_query_::SEARCH_IGNORE_INDEX_t<sub_expr_type>;

    static_assert(TL::Length<use_index_t>::value + TL::Length<ignore_index_t>::value ==  sub_expr_type::type_size , "");    

    record_range result;
    make_query_::SELECT_WITH_INDEX<use_index_t>::select(result, this, expr); //FIXME: support composite keys
    make_query_::SELECT_IGNORE_INDEX<ignore_index_t>::select(result, this, expr); //FIXME: support composite keys
    return result;
}

} // make
} // db
} // sdl

#endif // __SDL_SYSTEM_MAKETABLE_HPP__
