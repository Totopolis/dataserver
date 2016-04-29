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

using where_::operator_;
using where_::operator_t;
using where_::operator_list;

template<size_t i, class T, operator_ op> // T = where_::SEARCH
struct SEARCH_WHERE
{
    enum { offset = i };
    using type = T;
    using col = typename T::col;
    static const operator_ OP = op;
};

//--------------------------------------------------------------

template<class T, where_::condition cond = T::cond>
struct use_index {
    static_assert((T::hint != where_::INDEX::USE) || T::col::PK, "INDEX::USE require PK");
    enum { value = T::col::PK && (T::hint != where_::INDEX::IGNORE) };
};

template<class T> struct use_index<T, where_::condition::lambda>    { enum { value = false }; };
template<class T> struct use_index<T, where_::condition::order>     { enum { value = false }; };
template<class T> struct use_index<T, where_::condition::top>       { enum { value = false }; };

//--------------------------------------------------------------

template<class T, where_::condition cond = T::cond>
struct ignore_index {
    static_assert((T::hint != where_::INDEX::USE) || T::col::PK, "INDEX::USE require PK");
    enum { value = !T::col::PK || (T::hint == where_::INDEX::IGNORE) };
};

template<class T> struct ignore_index<T, where_::condition::lambda> { enum { value = true }; };
template<class T> struct ignore_index<T, where_::condition::order>  { enum { value = true }; };
template<class T> struct ignore_index<T, where_::condition::top>    { enum { value = false }; };

//--------------------------------------------------------------

template <class TList, class OList, size_t> struct search_use_index;
template <size_t i> struct search_use_index<NullType, NullType, i>
{
    using Index = NullType;
    using Types = NullType;
    using OList = NullType;
};

template <class Head, class Tail, operator_ OP, class NextOP, size_t i>
struct search_use_index<Typelist<Head, Tail>, operator_list<OP, NextOP>, i> {
private:
    enum { flag = use_index<Head>::value };
    using indx_i = typename Select<flag, Typelist<Int2Type<i>, NullType>, NullType>::Result;
    using type_i = typename Select<flag, Typelist<Head, NullType>, NullType>::Result;
    using oper_i = typename Select<flag, Typelist<operator_t<OP>, NullType>, NullType>::Result;
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

template <class Head, class Tail, operator_ OP, class NextOP, size_t i>
struct search_ignore_index<Typelist<Head, Tail>, operator_list<OP, NextOP>, i> {
private:
    enum { flag = ignore_index<Head>::value };
    using indx_i = typename Select<flag, Typelist<Int2Type<i>, NullType>, NullType>::Result;
    using type_i = typename Select<flag, Typelist<Head, NullType>, NullType>::Result;
    using oper_i = typename Select<flag, Typelist<operator_t<OP>, NullType>, NullType>::Result;
public:
    using Types = typename TL::Append<type_i, typename search_ignore_index<Tail, NextOP, i + 1>::Types>::Result; 
    using Index = typename TL::Append<indx_i, typename search_ignore_index<Tail, NextOP, i + 1>::Index>::Result;
    using OList = typename TL::Append<oper_i, typename search_ignore_index<Tail, NextOP, i + 1>::OList>::Result;
};

//--------------------------------------------------------------

template <template <condition> class compare, class TList, class OList, size_t>
struct search_condition;

template <template <condition> class compare, size_t i>
struct search_condition<compare, NullType, NullType, i>
{
    using Index = NullType;
    using Types = NullType;
    using OList = NullType;
};

template <template <condition> class compare, 
    class Head, class Tail, operator_ OP, class NextOP, size_t i>
struct search_condition<compare, Typelist<Head, Tail>, operator_list<OP, NextOP>, i> {
private:
    enum { flag = compare<Head::cond>::value };
    using indx_i = typename Select<flag, Typelist<Int2Type<i>, NullType>, NullType>::Result;
    using type_i = typename Select<flag, Typelist<Head, NullType>, NullType>::Result;
    using oper_i = typename Select<flag, Typelist<operator_t<OP>, NullType>, NullType>::Result;
public:
    using Types = typename TL::Append<type_i, typename search_condition<compare, Tail, NextOP, i + 1>::Types>::Result; 
    using Index = typename TL::Append<indx_i, typename search_condition<compare, Tail, NextOP, i + 1>::Index>::Result;
    using OList = typename TL::Append<oper_i, typename search_condition<compare, Tail, NextOP, i + 1>::OList>::Result;
};

//--------------------------------------------------------------

template<class Index, class TList, class OList> struct make_SEARCH_WHERE;
template<> struct make_SEARCH_WHERE<NullType, NullType, NullType>
{
    using Result = NullType;
};

template<size_t i, class NextIndex, class T, class NextType, operator_ OP,  class NextOP>
struct make_SEARCH_WHERE<
    Typelist<Int2Type<i>, NextIndex>,
    Typelist<T, NextType>,
    Typelist<operator_t<OP>, NextOP>>
{
private:
    using Item = Typelist<SEARCH_WHERE<i, T, OP>,  NullType>;
    using Next = typename make_SEARCH_WHERE<NextIndex, NextType, NextOP>::Result;
public:
    using Result = typename TL::Append<Item, Next>::Result;
};

//--------------------------------------------------------------

template <size_t i, class TList, class OList> struct append_SEARCH_WHERE;
template <size_t i> struct append_SEARCH_WHERE<i, NullType, NullType>
{
    using Result = NullType;
};

template<size_t i, class T, class NextType, operator_ OP, class NextOP>
struct append_SEARCH_WHERE<i, Typelist<T, NextType>, operator_list<OP, NextOP>>
{
private:
    using Item = Typelist<SEARCH_WHERE<i, T, OP>,  NullType>;
    using Next = typename append_SEARCH_WHERE<i+1, NextType, NextOP>::Result;
public:
    using Result = typename TL::Append<Item, Next>::Result;
};

//--------------------------------------------------------------

template<class sub_expr_type>
struct SEARCH_USE_INDEX {
private:
    using T = search_use_index<
        typename sub_expr_type::type_list,
        typename sub_expr_type::oper_list,
        0>;
    using Index = typename T::Index;
    using Types = typename T::Types;
    using OList = typename T::OList;     
public:
    using Result = typename make_SEARCH_WHERE<Index, Types, OList>::Result;
};

template<class T>
using SEARCH_USE_INDEX_t = typename SEARCH_USE_INDEX<T>::Result;

//--------------------------------------------------------------

template<class sub_expr_type>
struct SEARCH_IGNORE_INDEX {
private:
    using T = search_ignore_index<
        typename sub_expr_type::type_list,
        typename sub_expr_type::oper_list,
        0>;
    using Index = typename T::Index;
    using Types = typename T::Types;
    using OList = typename T::OList;     
public:
    using Result = typename make_SEARCH_WHERE<Index, Types, OList>::Result;
};

template<class T>
using SEARCH_IGNORE_INDEX_t = typename SEARCH_IGNORE_INDEX<T>::Result;

//--------------------------------------------------------------

template <class _search_where, bool is_limit>
struct SELECT_RECORD_WITH_INDEX {
private:
    template<class record_range, class query_type, class expr_type> static
    void select_cond(record_range & result, query_type * const query, expr_type const * const expr, condition_t<condition::WHERE>) {
    }
    template<class record_range, class query_type, class expr_type> 
    void select_cond(record_range & result, query_type * const query, expr_type const * const expr, condition_t<condition::IN>) const;

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
    template<class record_range>
    bool has_limit(record_range & result, Int2Type<1>) const {
        static_assert(is_limit, "");
        return this->limit <= result.size();
    }
    template<class record_range> static
    bool has_limit(record_range & result, Int2Type<0>) {
        static_assert(!is_limit, "");
        return false;
    }
public:
    const size_t limit;
    explicit SELECT_RECORD_WITH_INDEX(size_t lim): limit(lim) {
        SDL_ASSERT(is_limit == (limit > 0));
    }
    template<class record_range, class query_type, class sub_expr_type>
    void select(record_range & result, query_type * query, sub_expr_type const & expr) const;
};

template <class _search_where, bool is_limit>
template<class record_range, class query_type, class expr_type>
void SELECT_RECORD_WITH_INDEX<_search_where, is_limit>::select_cond(record_range & result, query_type * const query,
    expr_type const * const expr, condition_t<condition::IN>) const
{
    for (auto const & v : expr->value.values) {
        if (auto record = query->find_with_index(query->make_key(v))) {
            result.push_back(record);
            if (has_limit(result, Int2Type<is_limit>{}))
                return;
        }
    }
}

template <class _search_where, bool is_limit>
template<class record_range, class query_type, class sub_expr_type>
void SELECT_RECORD_WITH_INDEX<_search_where, is_limit>::select(record_range & result,
    query_type * query, sub_expr_type const & expr) const
{
    using T = _search_where;
    if (0) {
        SDL_TRACE("SELECT_RECORD_WITH_INDEX[off = ", T::offset, "] = ",
            where_::operator_name<T::OP>(), " ",
            where_::condition_name<T::type::cond>(), " ",
            typeid(typename T::type::col).name(),
            "\nlimit = ", this->limit
            );
    }
    select_cond(result, query, expr.get(Size2Type<T::offset>()), condition_t<T::type::cond>());
}

//--------------------------------------------------------------

template <class _search_where, bool is_limit>
struct SELECT_RECORD_NO_INDEX {
private:
    using search_col = typename _search_where::col;
private:
    template<class record, class value_type>
    static bool is_equal(record const & p, value_type const & v) {
        using search_val = typename search_col::val_type;
        return meta::is_equal<search_col>::equal(
                p.val(identity<search_col>{}), 
                static_cast<search_val const &>(v));
    }
    template<class query_type, class value_type> static
    typename query_type::record
    find(query_type * const query, value_type const & v, meta::is_key<true>) {
        A_STATIC_ASSERT_NOT_TYPE(void, search_col);
        return query->find_ignore_index(query->make_key(v));
    }
    template<class query_type, class value_type> static
    typename query_type::record
    find(query_type * const query, value_type const & v, meta::is_key<false>) {
        A_STATIC_ASSERT_NOT_TYPE(void, search_col);
        return query->find([&v](typename query_type::record p) {
            return is_equal(p, v);
        });
    }
private:
    template<class record_range, class query_type, class expr_type> static
    void select_cond(record_range & result, query_type * const query, expr_type const * const expr, condition_t<condition::WHERE>) {
    }
    template<class record_range, class query_type, class expr_type>
    void select_cond(record_range & result, query_type * const query, expr_type const * const expr, condition_t<condition::IN>) const;

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
    void select_cond(record_range & result, query_type * const query, expr_type const * const expr, condition_t<condition::lambda>) {
    }
    template<class record_range, class query_type, class expr_type> static
    void select_cond(record_range & result, query_type * const query, expr_type const * const expr, condition_t<condition::order>) {
    }
    template<class record_range>
    bool has_limit(record_range & result, Int2Type<1>) const {
        static_assert(is_limit, "");
        return this->limit <= result.size();
    }
    template<class record_range> static
    bool has_limit(record_range & result, Int2Type<0>) {
        static_assert(!is_limit, "");
        return false;
    }
public:
    const size_t limit;
    explicit SELECT_RECORD_NO_INDEX(size_t lim): limit(lim) {
        SDL_ASSERT(is_limit == (limit > 0));
    }
    template<class record_range, class query_type, class sub_expr_type>
    void select(record_range & result, query_type * query, sub_expr_type const & expr) const;
};

template <class _search_where, bool is_limit>
template<class record_range, class query_type, class expr_type>
void SELECT_RECORD_NO_INDEX<_search_where, is_limit>::select_cond(record_range & result, 
    query_type * const query, expr_type const * const expr, condition_t<condition::IN>) const
{
    query->scan_if([this, expr, &result](typename query_type::record p){
        for (auto const & v : expr->value.values) {
            if (is_equal(p, v)) {
                result.push_back(p);
                if (has_limit(result, Int2Type<is_limit>{}))
                    return false;
            }
        }
        return true;
    });
}

template <class _search_where, bool is_limit>
template<class record_range, class query_type, class sub_expr_type>
void SELECT_RECORD_NO_INDEX<_search_where, is_limit>::select(record_range & result,
    query_type * query, sub_expr_type const & expr) const 
{
    using T = _search_where;
    if (0) {
        SDL_TRACE("SELECT_RECORD_NO_INDEX[off = ", T::offset, "] = ",
            where_::operator_name<T::OP>(), " ",
            where_::condition_name<T::type::cond>(), " ",
            typeid(typename T::type::col).name(),
            "\nlimit = ", this->limit
            );
    }
    select_cond(result, query, expr.get(Size2Type<T::offset>()), condition_t<T::type::cond>());
}

//--------------------------------------------------------------

template <class search_list, bool is_limit> struct SELECT_WITH_INDEX;
template <bool is_limit> struct SELECT_WITH_INDEX<NullType, is_limit>
{
    template<class record_range, class query_type, class sub_expr_type> static
    void select(record_range &, query_type *, sub_expr_type const &, size_t) {}
};

template<class T, class NextType, bool is_limit> // T = search_key
struct SELECT_WITH_INDEX<Typelist<T, NextType>, is_limit>
{
    template<class record_range, class query_type, class sub_expr_type> static
    void select(record_range & result, query_type * query, sub_expr_type const & expr, const size_t limit) {
        SELECT_RECORD_WITH_INDEX<T, is_limit>(limit).select(result, query, expr);
        SELECT_WITH_INDEX<NextType, is_limit>::select(result, query, expr, limit);
    }
};

//--------------------------------------------------------------

template <class search_list, bool is_limit> struct SELECT_NO_INDEX;
template <bool is_limit> struct SELECT_NO_INDEX<NullType, is_limit>
{
    template<class record_range, class query_type, class sub_expr_type> static
    void select(record_range &, query_type *, sub_expr_type const &, size_t) {}
};

template<class T, class NextType, bool is_limit> // T = search_key
struct SELECT_NO_INDEX<Typelist<T, NextType>, is_limit>
{
    template<class record_range, class query_type, class sub_expr_type> static
    void select(record_range & result, query_type * query, sub_expr_type const & expr, const size_t limit) {
        SELECT_RECORD_NO_INDEX<T, is_limit>(limit).select(result, query, expr);
        SELECT_NO_INDEX<NextType, is_limit>::select(result, query, expr, limit);
    }
};

//--------------------------------------------------------------

template <operator_ OP, class TList>
struct search_operator;

template <operator_ OP>
struct search_operator<OP, NullType>
{
    using Result = NullType;
};

template <operator_ OP, class Head, class Tail>
struct search_operator<OP, Typelist<Head, Tail>> {
private:
    using Item = typename Select<Head::OP == OP, Typelist<Head, NullType>, NullType>::Result;
public:
    using Result = typename TL::Append<Item, typename search_operator<OP, Tail>::Result>::Result; 
};

template <operator_ OP, class TList>
using search_operator_t = typename search_operator<OP, TList>::Result;

//--------------------------------------------------------------

template<class T>       // T = SEARCH_WHERE 
struct RECORD_SELECT {
private:
    template<class record, class value_type>
    static bool is_equal(record const & p, value_type const & v) {
        using col = typename T::col;
        return meta::is_equal<col>::equal(
                p.val(identity<col>{}), 
                static_cast<typename col::val_type const &>(v));
    }
    template<sortorder ord, class record, class value_type>
    static bool is_less(record const & p, value_type const & v) {
        using col = typename T::col;
        return meta::col_less<col, ord>::less(
                p.val(identity<col>{}), 
                static_cast<typename col::val_type const &>(v));
    }
    template<class record, class expr_type> static bool select(record const & p, expr_type const * const expr, condition_t<condition::WHERE>);
    template<class record, class expr_type> static bool select(record const & p, expr_type const * const expr, condition_t<condition::IN>);
    template<class record, class expr_type> static bool select(record const & p, expr_type const * const expr, condition_t<condition::NOT>);
    template<class record, class expr_type> static bool select(record const & p, expr_type const * const expr, condition_t<condition::LESS>);
    template<class record, class expr_type> static bool select(record const & p, expr_type const * const expr, condition_t<condition::GREATER>);
    template<class record, class expr_type> static bool select(record const & p, expr_type const * const expr, condition_t<condition::LESS_EQ>);
    template<class record, class expr_type> static bool select(record const & p, expr_type const * const expr, condition_t<condition::GREATER_EQ>);
    template<class record, class expr_type> static bool select(record const & p, expr_type const * const expr, condition_t<condition::BETWEEN>);
    template<class record, class expr_type> static bool select(record const & p, expr_type const * const expr, condition_t<condition::lambda>);
public:
    template<class record, class sub_expr_type> static
    bool select(record const & p, sub_expr_type const & expr) {
        return select(p, expr.get(Size2Type<T::offset>()), condition_t<T::type::cond>{});
    }
};

template<class T>
template<class record, class expr_type> inline
bool RECORD_SELECT<T>::select(record const & p, expr_type const * const expr, condition_t<condition::WHERE>) {
    return is_equal(p, expr->value.values);
}

template<class T>
template<class record, class expr_type> inline
bool RECORD_SELECT<T>::select(record const & p, expr_type const * const expr, condition_t<condition::IN>) {
    for (auto & v : expr->value.values) {
        if (is_equal(p, v))
            return true;
    }
    return false;
}

template<class T>
template<class record, class expr_type> inline
bool RECORD_SELECT<T>::select(record const & p, expr_type const * const expr, condition_t<condition::NOT>) {
    for (auto & v : expr->value.values) {
        if (is_equal(p, v))
            return false;
    }
    return true;
}

template<class T>
template<class record, class expr_type> inline
bool RECORD_SELECT<T>::select(record const & p, expr_type const * const expr, condition_t<condition::LESS>) {
    return is_less<sortorder::ASC>(p, expr->value.values);
}

template<class T>
template<class record, class expr_type> inline
bool RECORD_SELECT<T>::select(record const & p, expr_type const * const expr, condition_t<condition::GREATER>) {
    return is_less<sortorder::DESC>(p, expr->value.values);
}

template<class T>
template<class record, class expr_type> inline
bool RECORD_SELECT<T>::select(record const & p, expr_type const * const expr, condition_t<condition::LESS_EQ>) {
    return is_equal(p, expr->value.values) || is_less<sortorder::ASC>(p, expr->value.values);
}

template<class T>
template<class record, class expr_type> inline
bool RECORD_SELECT<T>::select(record const & p, expr_type const * const expr, condition_t<condition::GREATER_EQ>) {
    return is_equal(p, expr->value.values) || is_less<sortorder::DESC>(p, expr->value.values);
}

template<class T>
template<class record, class expr_type> inline
bool RECORD_SELECT<T>::select(record const & p, expr_type const * const expr, condition_t<condition::BETWEEN>) {
    return (is_equal(p, expr->value.values.first) || is_less<sortorder::DESC>(p, expr->value.values.first))
        && (is_equal(p, expr->value.values.second) || is_less<sortorder::ASC>(p, expr->value.values.second));
}

template<class T>
template<class record, class expr_type> inline
bool RECORD_SELECT<T>::select(record const & p, expr_type const * const expr, condition_t<condition::lambda>) {
    return expr->value(p);
}

//--------------------------------------------------------------

template<class TList, bool Result> struct SELECT_OR;
template<bool Result> struct SELECT_OR<NullType, Result>
{
    template<class record, class sub_expr_type> static
    bool select(record const &, sub_expr_type const &) {
        return Result;
    }
};

template<class T, class NextType, bool Result>
struct SELECT_OR<Typelist<T, NextType>, Result> // T = SEARCH_WHERE
{
    template<class record, class sub_expr_type> static
    bool select(record const & p, sub_expr_type const & expr) {
        return RECORD_SELECT<T>::select(p, expr) ||
            SELECT_OR<NextType, false>::select(p, expr);
    }
};

//--------------------------------------------------------------

template<class TList, bool Result> struct SELECT_AND;
template<bool Result> struct SELECT_AND<NullType, Result>
{
    template<class record, class sub_expr_type> static
    bool select(record const &, sub_expr_type const &) {
        return Result;
    }
};

template<class T, class NextType, bool Result>
struct SELECT_AND<Typelist<T, NextType>, Result> // T = SEARCH_WHERE
{
    template<class record, class sub_expr_type> static
    bool select(record const & p, sub_expr_type const & expr) {
        return RECORD_SELECT<T>::select(p, expr) && 
            SELECT_AND<NextType, true>::select(p, expr);
    }
};

//--------------------------------------------------------------

template<class _search_where_list, bool is_limit>
struct SCAN_TABLE {
private:
    template<class record_range>
    bool has_limit(record_range & result, Int2Type<1>) const {
        static_assert(is_limit, "");
        return this->limit <= result.size();
    }
    template<class record_range> static
    bool has_limit(record_range & result, Int2Type<0>) {
        static_assert(!is_limit, "");
        return false;
    }
    template<class record, class sub_expr_type> static
    bool is_select(record const & p, sub_expr_type const & expr) {
        using AND = search_operator_t<operator_::AND, _search_where_list>;
        using OR = search_operator_t<operator_::OR, _search_where_list>;
        static_assert(TL::Length<AND>::value + TL::Length<OR>::value, "");
        return SELECT_AND<AND, true>::select(p, expr)
            && SELECT_OR<OR, true>::select(p, expr);
    }
public:
    const size_t limit;
    explicit SCAN_TABLE(size_t lim): limit(lim) {
        SDL_ASSERT(is_limit == (limit > 0));
    }
    template<class record_range, class query_type, class sub_expr_type>
    void select(record_range & result, query_type * query, sub_expr_type const & expr) const {
        query->scan_if([this, &expr, &result](typename query_type::record p){
            if (is_select(p, expr)) {
                result.push_back(p);
                if (has_limit(result, Int2Type<is_limit>{}))
                    return false;
            }
            return true;
        });
    }
};

//--------------------------------------------------------------

template<class sub_expr_type>
inline size_t _TOP(sub_expr_type const & expr, identity<NullType>){
    return 0;
}

template<class sub_expr_type, class T>
inline size_t _TOP(sub_expr_type const & expr, identity<T>)
{
    static_assert(TL::Length<T>::value == 1, "");
    enum { offset = T::Head::offset };    
    A_STATIC_ASSERT_TYPE(size_t, where_::TOP::value_type);
    return expr.get(Size2Type<offset>())->value;
}

} // make_query_

template<class this_table, class record>
template<class sub_expr_type>
typename make_query<this_table, record>::record_range
make_query<this_table, record>::VALUES(sub_expr_type const & expr)
{
    using namespace make_query_;

    SDL_TRACE("\nVALUES:");
    if (1) {
        where_::trace_::trace_sub_expr(expr);
    }
    record_range result;

    using TS_TOP = search_condition<
        where_::is_condition_top,
        typename sub_expr_type::type_list,
        typename sub_expr_type::oper_list,
        0>;

    using TS_ORDER = search_condition<
        where_::is_condition_order,
        typename sub_expr_type::type_list,
        typename sub_expr_type::oper_list,
        0>;

    using TS_SEARCH = search_condition<
        where_::is_condition_SEARCH,
        typename sub_expr_type::type_list,
        typename sub_expr_type::oper_list,
        0>;

    using TS_TOP_WHERE = typename make_SEARCH_WHERE<
        typename TS_TOP::Index,
        typename TS_TOP::Types,
        typename TS_TOP::OList
    >::Result;

    using TS_ORDER_WHERE = typename make_SEARCH_WHERE<
        typename TS_ORDER::Index,
        typename TS_ORDER::Types,
        typename TS_ORDER::OList
    >::Result;

    using TS_SEARCH_WHERE = typename make_SEARCH_WHERE<
        typename TS_SEARCH::Index,
        typename TS_SEARCH::Types,
        typename TS_SEARCH::OList
    >::Result;

    static_assert(TL::Length<TS_TOP_WHERE>::value < 2, "TOP");
    static_assert(TL::Length<TS_SEARCH_WHERE>::value, "SEARCH");

    using USE_IDX = SEARCH_USE_INDEX_t<sub_expr_type>;   
    auto const limit = _TOP(expr, identity<TS_TOP_WHERE>{});
    
    if (1) {
        if (limit)
            SCAN_TABLE<TS_SEARCH_WHERE, true>(limit).select(result, this, expr); else
            SCAN_TABLE<TS_SEARCH_WHERE, false>(limit).select(result, this, expr);
    }
    else if (TL::Length<USE_IDX>::value) {
        SDL_TRACE(typeid(USE_IDX).name());
        if (limit)
            SELECT_WITH_INDEX<USE_IDX, true>::select(result, this, expr, limit); else
            SELECT_WITH_INDEX<USE_IDX, false>::select(result, this, expr, limit);
    }
    else {
        if (limit)
            SELECT_NO_INDEX<TS_SEARCH_WHERE, true>::select(result, this, expr, limit); else
            SELECT_NO_INDEX<TS_SEARCH_WHERE, false>::select(result, this, expr, limit);
    }
    if (TL::Length<TS_ORDER_WHERE>::value) {
    }
    return result;
}

} // make
} // db
} // sdl

#endif // __SDL_SYSTEM_MAKETABLE_HPP__

#if 0 // reserved
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
#endif