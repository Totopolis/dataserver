// maketable.hpp
//
#pragma once
#ifndef __SDL_SYSTEM_MAKETABLE_HPP__
#define __SDL_SYSTEM_MAKETABLE_HPP__

#if SDL_DEBUG && defined(SDL_OS_WIN32)
#define SDL_TRACE_QUERY(...)    SDL_TRACE(__VA_ARGS__)
#define SDL_DEBUG_QUERY         1
#else
#define SDL_TRACE_QUERY(...)    ((void)0)
#define SDL_DEBUG_QUERY         0
#endif

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

template <condition c>
struct is_use_index {
    enum { value = (c < condition::lambda) };
};

template<class T, size_t key_pos, bool search = is_use_index<T::cond>::value>
struct use_index;

template<class T, size_t key_pos>
struct use_index<T, key_pos, true> {
    static_assert((T::hint != where_::INDEX::USE) || T::col::PK, "INDEX::USE need primary key");
    enum { value = T::col::PK && (T::col::key_pos == key_pos) && (T::hint != where_::INDEX::IGNORE) };
};

template<class T, size_t key_pos>
struct use_index<T, key_pos, false> {
    enum { value = false };
};

//--------------------------------------------------------------

template<class T, size_t key_pos, bool search = is_use_index<T::cond>::value>
struct ignore_index;

template<class T, size_t key_pos>
struct ignore_index<T, key_pos, true> {
    enum { value = !use_index<T, key_pos, true>::value };
};

template<class T, size_t key_pos>
struct ignore_index<T, key_pos, false> {
    enum { value = false };
};

//--------------------------------------------------------------

template <class TList, class OList> struct check_index;
template <> struct check_index<NullType, NullType>
{
    enum { value = true };
};

template <class Head, class Tail, operator_ OP, class NextOP>
struct check_index<Typelist<Head, Tail>, operator_list<OP, NextOP>> {
private:
    enum { check = use_index<Head, 0>::value };
public:
    enum { value = check_index<Tail, NextOP>::value || check };
};

template<class sub_expr_type>
struct CHECK_INDEX 
{
    enum { value = check_index<
        typename sub_expr_type::type_list,
        typename sub_expr_type::oper_list>::value };
};

//--------------------------------------------------------------

namespace search_key_ {

template<template <class, operator_> class select, class TList, class OList, size_t>
struct search_key;

template <template <class, operator_> class select, size_t i> 
struct search_key<select, NullType, NullType, i>
{
    using Result = NullType;
};

template <template <class, operator_> class select, 
    class Head, class Tail, operator_ OP, class NextOP, size_t i>
struct search_key<select, Typelist<Head, Tail>, operator_list<OP, NextOP>, i> {
private:
    enum { value = select<Head, OP>::value };

    using T1 = Typelist<SEARCH_WHERE<i, Head, OP>, NullType>;
    using T2 = Select_t<value, T1, NullType>;
   
    using Next = search_key<select, Tail, NextOP, i + 1>;
public:
    using Result = TL::Append_t<T2, typename Next::Result>;
};

//--------------------------------------------------------------

template <class T, operator_ OP, size_t key_pos, operator_ key_op> // T = where_::SEARCH
struct select_key {
private:
    enum { temp = use_index<T, key_pos>::value };
public:
    enum { value = temp && (OP == key_op) };
};

template <class T, operator_ OP, size_t key_pos, operator_ key_op> // T = where_::SEARCH
struct select_no_key {
private:
    enum { temp = ignore_index<T, key_pos>::value };
public:
    enum { value = temp && (OP == key_op) };
};

template <class T, operator_ OP, operator_ key_op> // T = where_::SEARCH
struct select_lambda
{
    enum { value = where_::is_condition_lambda<T::cond>::value && (OP == key_op) };
};

//--------------------------------------------------------------

template<class TList, class T> struct join_key;

template <class T> struct join_key<NullType, T> {
    using Result = NullType;
public:
    static void trace(){}
};

template <class Head, class Tail, class T>
struct join_key<Typelist<Head, Tail>, T> {
private:
    static_assert(!IsTypelist<Head>::value, "");
    static_assert(IsTypelist<T>::value, "");

    struct key_type {
        using type_list = Typelist<Head, T>;
        static_assert(TL::Length<type_list>::value == TL::Length<T>::value + 1, "");
    };
public:
    using Result = Typelist<key_type, typename join_key<Tail, T>::Result>;

#if SDL_DEBUG_QUERY
    static void trace(){
        SDL_TRACE_QUERY("\njoin_key = ", TL::Length<Result>::value);
        meta::trace_typelist<typename key_type::type_list>();
        join_key<Tail, T>::trace();
    }
#endif
};

template<class T1, class T2>
using join_key_t = typename join_key<T1, T2>::Result;

//--------------------------------------------------------------

struct SEARCH_KEY_BASE {
protected:
    template <class T, operator_ OP> using _key_OR_0 = select_key<T, OP, 0, operator_::OR>;
    template <class T, operator_ OP> using _key_AND_0 = select_key<T, OP, 0, operator_::AND>;
    template <class T, operator_ OP> using _key_AND_1 = select_key<T, OP, 1, operator_::AND>;
    template <class T, operator_ OP> using _no_key_OR_0 = select_no_key<T, OP, 0, operator_::OR>;
    template <class T, operator_ OP> using _no_key_AND_0 = select_no_key<T, OP, 0, operator_::AND>;
    template <class T, operator_ OP> using _no_key_AND_1 = select_no_key<T, OP, 1, operator_::AND>;
    template <class T, operator_ OP> using _lambda_OR = select_lambda<T, OP, operator_::OR>;
    template <class T, operator_ OP> using _lambda_AND = select_lambda<T, OP, operator_::AND>;
};

template<class sub_expr_type>
struct SEARCH_KEY : SEARCH_KEY_BASE 
{
    using key_OR_0 = typename search_key<_key_OR_0,
        typename sub_expr_type::type_list,
        typename sub_expr_type::oper_list,
        0
    >::Result;

    using key_AND_0 = typename search_key<_key_AND_0,
        typename sub_expr_type::type_list,
        typename sub_expr_type::oper_list,
        0
    >::Result;

    using key_AND_1 = typename search_key<_key_AND_1,
        typename sub_expr_type::type_list,
        typename sub_expr_type::oper_list,
        0
    >::Result;

    using no_key_OR_0 = typename search_key<_no_key_OR_0,
        typename sub_expr_type::type_list,
        typename sub_expr_type::oper_list,
        0
    >::Result;

    using no_key_AND_0 = typename search_key<_no_key_AND_0,
        typename sub_expr_type::type_list,
        typename sub_expr_type::oper_list,
        0
    >::Result;

    using no_key_AND_1 = typename search_key<_no_key_AND_1,
        typename sub_expr_type::type_list,
        typename sub_expr_type::oper_list,
        0
    >::Result;

    using lambda_OR = typename search_key<_lambda_OR,
        typename sub_expr_type::type_list,
        typename sub_expr_type::oper_list,
        0
    >::Result;

    using lambda_AND = typename search_key<_lambda_AND,
        typename sub_expr_type::type_list,
        typename sub_expr_type::oper_list,
        0
    >::Result;

    using Result = Select_t<TL::Length<key_AND_1>::value, join_key_t<key_OR_0, key_AND_1>, key_OR_0>;

    static_assert(TL::Length<Result>::value == TL::Length<key_OR_0>::value, "");

#if SDL_DEBUG_QUERY
    static void trace() {
        SDL_TRACE_QUERY("\nSEARCH_KEY = ", TL::Length<Result>::value);
        SDL_TRACE_QUERY("key_OR_0 = ", TL::Length<key_OR_0>::value, " ", meta::short_name<key_OR_0>());
        SDL_TRACE_QUERY("key_AND_0 = ", TL::Length<key_AND_0>::value, " ", meta::short_name<key_AND_0>());
        SDL_TRACE_QUERY("key_AND_1 = ", TL::Length<key_AND_1>::value, " ", meta::short_name<key_AND_1>());
        SDL_TRACE_QUERY("no_key_OR_0 = ", TL::Length<no_key_OR_0>::value, " ", meta::short_name<no_key_OR_0>());
        SDL_TRACE_QUERY("no_key_AND_0 = ", TL::Length<no_key_AND_0>::value, " ", meta::short_name<no_key_AND_0>());
        SDL_TRACE_QUERY("no_key_AND_1 = ", TL::Length<no_key_AND_1>::value, " ", meta::short_name<no_key_AND_1>());
        SDL_TRACE_QUERY("lambda_OR = ", TL::Length<lambda_OR>::value, " ", meta::short_name<lambda_OR>());
        SDL_TRACE_QUERY("lambda_AND = ", TL::Length<lambda_AND>::value, " ", meta::short_name<lambda_AND>());
        join_key<key_OR_0, key_AND_1>::trace();
    }
#endif
};

} // search_key_ 

using search_key_::SEARCH_KEY;

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

template <template <condition> class compare, class Head, class Tail, operator_ OP, class NextOP, size_t i>
struct search_condition<compare, Typelist<Head, Tail>, operator_list<OP, NextOP>, i> {
private:
    enum { flag = compare<Head::cond>::value };
    using indx_i = Select_t<flag, Typelist<Int2Type<i>, NullType>, NullType>;
    using type_i = Select_t<flag, Typelist<Head, NullType>, NullType>;
    using oper_i = Select_t<flag, Typelist<operator_t<OP>, NullType>, NullType>;
public:
    using Types = TL::Append_t<type_i, typename search_condition<compare, Tail, NextOP, i + 1>::Types>; 
    using Index = TL::Append_t<indx_i, typename search_condition<compare, Tail, NextOP, i + 1>::Index>;
    using OList = TL::Append_t<oper_i, typename search_condition<compare, Tail, NextOP, i + 1>::OList>;
};

//--------------------------------------------------------------

template<class Index, class TList, class OList>
struct make_SEARCH_WHERE;

template<> struct make_SEARCH_WHERE<NullType, NullType, NullType>
{
    using Result = NullType;
};

template<size_t i, class NextIndex, class T, class NextType, operator_ OP, class NextOP>
struct make_SEARCH_WHERE<
    Typelist<Int2Type<i>, NextIndex>,
    Typelist<T, NextType>,
    Typelist<operator_t<OP>, NextOP>>
{
private:
    using Item = Typelist<SEARCH_WHERE<i, T, OP>,  NullType>;
    using Next = typename make_SEARCH_WHERE<NextIndex, NextType, NextOP>::Result;
public:
    using Result = TL::Append_t<Item, Next>;
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
    using Item = Select_t<Head::OP == OP, Typelist<Head, NullType>, NullType>;
public:
    using Result = TL::Append_t<Item, typename search_operator<OP, Tail>::Result>; 
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

template<class _search_where_list, bool is_limit = false>
struct SCAN_TABLE {
private:
    using search_AND = search_operator_t<operator_::AND, _search_where_list>;
    using search_OR = search_operator_t<operator_::OR, _search_where_list>;
    static_assert(TL::Length<search_OR>::value, "empty OR");

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
        return
            SELECT_OR<search_OR, true>::select(p, expr) &&    // any of 
            SELECT_AND<search_AND, true>::select(p, expr);    // must be
    }
public:
    const size_t limit;
    explicit SCAN_TABLE(size_t lim = 0): limit(lim) {
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

template<class _search_where_list, bool is_limit>
struct SEEK_TABLE {
private:
public:
    const size_t limit;
    explicit SEEK_TABLE(size_t lim): limit(lim) {
        SDL_ASSERT(is_limit == (limit > 0));
    }
    template<class record_range, class query_type, class sub_expr_type>
    void select(record_range & result, query_type * query, sub_expr_type const & expr) const {
        using Impl = SCAN_TABLE<_search_where_list, is_limit>;
        Impl(limit).select(result, query, expr);
    }
};

//--------------------------------------------------------------

template<class sub_expr_type>
struct SELECT_TOP_TYPE {
private:
    using TOP_1 = search_condition<
        where_::is_condition_top,
        typename sub_expr_type::type_list,
        typename sub_expr_type::oper_list,
        0>;

    using TOP_2 = typename make_SEARCH_WHERE<
        typename TOP_1::Index,
        typename TOP_1::Types,
        typename TOP_1::OList
    >::Result;
public:
    using Result = TOP_2;
private:
    static size_t value(sub_expr_type const & expr, identity<NullType>) {
        return 0;
    }
    template<class T>
    static size_t value(sub_expr_type const & expr, identity<T>) {
        static_assert(TL::Length<T>::value == 1, "TOP duplicate");
        A_STATIC_ASSERT_TYPE(size_t, where_::TOP::value_type);
        return expr.get(Size2Type<T::Head::offset>())->value;
    }
public:
    static size_t value(sub_expr_type const & expr) {
        return value(expr, identity<Result>{});
    }
};

template<class sub_expr_type>
inline size_t SELECT_TOP(sub_expr_type const & expr) {
    using T = SELECT_TOP_TYPE<sub_expr_type>;
    static_assert(TL::Length<typename T::Result>::value == 1, "SELECT_TOP");
    return T::value(expr);
}

//--------------------------------------------------------------

template <class TList, class col> struct col_index;

template <class col>
struct col_index<NullType, col>
{
    enum { value = -1 };
};

template <class Head, class Tail, class col>
struct col_index<Typelist<Head, Tail>, col> { // Head = SEARCH_WHERE<where_::ORDER_BY>
private:
    enum { found = std::is_same<typename Head::col, col>::value };
    enum { temp = col_index<Tail, col>::value }; // can be -1
public: 
    enum { value = found ? 0 : ((temp == -1) ? -1 : (1 + temp)) };
};

//--------------------------------------------------------------

template<class TList> struct CHECK_ORDER;

template<> struct CHECK_ORDER<NullType> {
    enum { value = true };
};

template<class Head, class Tail>
struct CHECK_ORDER<Typelist<Head, Tail>> { // Head = SEARCH_WHERE<where_::ORDER_BY>
private:
    static_assert(Head::OP == operator_::AND, "ORDER_BY require AND");
    enum { found = col_index<Tail, typename Head::col>::value }; 
public:
    enum { value = (found == -1) && CHECK_ORDER<Tail>::value };
    static_assert(value, "ORDER_BY dublicate column");
};

//--------------------------------------------------------------

template <class TList, sortorder> struct order_cluster;
template <sortorder ord> struct order_cluster<NullType, ord>
{
    using Result = NullType;
};

template <class T, class Tail, sortorder ord>
struct order_cluster< Typelist<T, Tail>, ord> { // T = SEARCH_WHERE<where_::ORDER_BY>
private:
    enum { value = T::col::PK && (0 == T::col::key_pos) && (T::type::value == ord) };
public:
    using Result = Select_t<value, T, typename order_cluster<Tail, ord>::Result>;
};

template<class sub_expr_type>
struct SELECT_ORDER_TYPE {
private:
    using ORDER_1 = search_condition<
        where_::is_condition_order,
        typename sub_expr_type::type_list,
        typename sub_expr_type::oper_list,
        0>;
    using ORDER_2 = typename make_SEARCH_WHERE<
        typename ORDER_1::Index,
        typename ORDER_1::Types,
        typename ORDER_1::OList
    >::Result;

    enum { check = CHECK_ORDER<ORDER_2>::value };
    static_assert(check, "SELECT_ORDER_TYPE");

    using temp = typename order_cluster<ORDER_2, sortorder::ASC>::Result;
    enum { remove = (TL::IndexOf<ORDER_2, temp>::value == 0) && (TL::Length<ORDER_2>::value == 1) };

    using ORDER_3 = Select_t<remove, NullType, ORDER_2>;
public:
    using Result = ORDER_3;
};

//--------------------------------------------------------------

template <condition c>
struct is_SEARCH {
    enum { value = (c <= condition::lambda) };
};

template<class sub_expr_type>
struct SELECT_SEARCH_TYPE {
private:
    using T = search_condition<
        is_SEARCH,
        typename sub_expr_type::type_list,
        typename sub_expr_type::oper_list,
        0>;
public:
    using Result = typename make_SEARCH_WHERE<
        typename T::Index,
        typename T::Types,
        typename T::OList
    >::Result;
    static_assert(TL::Length<Result>::value, "SEARCH");
};

//--------------------------------------------------------------

enum class stable_sort { false_, true_ };

template<class col, sortorder order, stable_sort _sort> 
struct record_sort
{
    template<class record_range>
    static void sort(record_range & range) {
        static_assert(_sort == stable_sort::false_, "");
        using record = typename record_range::value_type;
        std::sort(range.begin(), range.end(), [](record const & x, record const & y){
            return meta::col_less<col, order>::less(
                x.val(identity<col>{}), 
                y.val(identity<col>{}));
        });
    }
};

template<class col, sortorder order> 
struct record_sort<col, order, stable_sort::true_>
{
    template<class record_range>
    static void sort(record_range & range) {
        using record = typename record_range::value_type;
        std::stable_sort(range.begin(), range.end(), [](record const & x, record const & y){
            return meta::col_less<col, order>::less(
                x.val(identity<col>{}), 
                y.val(identity<col>{}));
        });
    }
};

//--------------------------------------------------------------

template<class TList> struct SORT_RECORD_RANGE;
template<> struct SORT_RECORD_RANGE<NullType>
{
    template<class record_range>
    static void sort(record_range &){}
};

template<class Head>
struct SORT_RECORD_RANGE<Typelist<Head, NullType>>
{
    template<class record_range>
    static void sort(record_range & range) {
        record_sort<typename Head::col, Head::type::value, stable_sort::false_>::sort(range);
    }
};

template<class Head, class Tail>
struct SORT_RECORD_RANGE<Typelist<Head, Tail>>  // Head = SEARCH_WHERE<where_::ORDER_BY>
{
    template<class record_range>
    static void sort(record_range & range) {
        A_STATIC_ASSERT_NOT_TYPE(Tail, NullType);
        SORT_RECORD_RANGE<Tail>::sort(range);
        record_sort<typename Head::col, Head::type::value, stable_sort::true_>::sort(range);
    }
};

//--------------------------------------------------------------

template<class sub_expr_type, class TOP = NullType>
struct SCAN_OR_SEEK {
private:
    enum { is_limit = !IsNullType<TOP>::value };    
    static size_t limit(sub_expr_type const &, identity<NullType>) {
        return 0;
    }
    template<class T> static size_t limit(sub_expr_type const & expr, identity<T>) { 
        return SELECT_TOP(expr);
    }
    static size_t limit(sub_expr_type const & expr) {
        return limit(expr, identity<TOP>{});
    }
private:
    using SEARCH = typename SELECT_SEARCH_TYPE<sub_expr_type>::Result;
    using KEYS = SEARCH_KEY<sub_expr_type>;
public:
    template<class record_range, class query_type> static
    void select(record_range & result, query_type * query, sub_expr_type const & expr);
};

template<class sub_expr_type, class TOP>
template<class record_range, class query_type>
void SCAN_OR_SEEK<sub_expr_type, TOP>::select(record_range & result, query_type * query, sub_expr_type const & expr)
{
    enum { is_lambda_OR = TL::Length<typename KEYS::lambda_OR>::value != 0 };
    enum { is_key_OR_0 = TL::Length<typename KEYS::key_OR_0>::value != 0 };
    enum { is_key_AND_0 = TL::Length<typename KEYS::key_AND_0>::value != 0 };

    enum { use_Seek = (is_key_OR_0 || is_key_AND_0) && !is_lambda_OR };

    SDL_TRACE_QUERY("is_lambda_OR = ", is_lambda_OR);
    SDL_TRACE_QUERY("is_key_OR_0 = ", is_key_OR_0);
    SDL_TRACE_QUERY("is_key_AND_0 = ", is_key_AND_0);
    SDL_TRACE_QUERY("use_Seek = ", use_Seek);

    using Scan = SCAN_TABLE<SEARCH, is_limit>;
    using Seek = SEEK_TABLE<SEARCH, is_limit>;
    using Impl = Select_t<use_Seek, Seek, Scan>;

    Impl(limit(expr)).select(result, query, expr);
}

//--------------------------------------------------------------

template<class sub_expr_type, class TOP, class ORDER>
struct QUERY_VALUES
{
    static_assert(TL::Length<TOP>::value == 1, "TOP");
    static_assert(TL::Length<ORDER>::value, "ORDER");

    template<class record_range, class query_type> static
    void select(record_range & result, query_type * query, sub_expr_type const & expr) {
        SCAN_OR_SEEK<sub_expr_type>::select(result, query, expr);
        SORT_RECORD_RANGE<ORDER>::sort(result);
        result.resize(a_min(SELECT_TOP(expr), result.size()));
        result.shrink_to_fit();
    }
};

template<class sub_expr_type>
struct QUERY_VALUES<sub_expr_type, NullType, NullType>
{
    template<class record_range, class query_type> static
    void select(record_range & result, query_type * query, sub_expr_type const & expr) {
        SCAN_OR_SEEK<sub_expr_type>::select(result, query, expr);
    }
};

template<class sub_expr_type, class TOP>
struct QUERY_VALUES<sub_expr_type, TOP, NullType>
{
    template<class record_range, class query_type> static
    void select(record_range & result, query_type * query, sub_expr_type const & expr) {
        SCAN_OR_SEEK<sub_expr_type, TOP>::select(result, query, expr);
    }
};

template<class sub_expr_type, class ORDER>
struct QUERY_VALUES<sub_expr_type, NullType, ORDER> {
private:
    using SEARCH = typename SELECT_SEARCH_TYPE<sub_expr_type>::Result;
    using KEYS = SEARCH_KEY<sub_expr_type>;
public:
    template<class record_range, class query_type> static
    void select(record_range & result, query_type * query, sub_expr_type const & expr) {
        SCAN_TABLE<SEARCH>().select(result, query, expr);
        SORT_RECORD_RANGE<ORDER>::sort(result);
    }
};

} // make_query_

//--------------------------------------------------------------

template<class this_table, class record>
template<class sub_expr_type>
typename make_query<this_table, record>::record_range
make_query<this_table, record>::VALUES(sub_expr_type const & expr)
{
    using namespace make_query_;

    static_assert(CHECK_INDEX<sub_expr_type>::value, "");
    static_assert(index_size <= 2, "TODO: SEARCH_KEY");

    if (1) {
        SDL_TRACE_QUERY("\nVALUES:");
        where_::trace_::trace_sub_expr(expr);
    }
    using TOP = typename SELECT_TOP_TYPE<sub_expr_type>::Result;
    using ORDER = typename SELECT_ORDER_TYPE<sub_expr_type>::Result;

    record_range result;
    QUERY_VALUES<sub_expr_type, TOP, ORDER>::select(result, this, expr);
    return result;
}

} // make
} // db
} // sdl

#endif // __SDL_SYSTEM_MAKETABLE_HPP__

#if 0
template<class T1, class T2> struct make_pair;

template<> struct make_pair<NullType, NullType> {
    using Result = NullType;
};

template<class T> struct make_pair<NullType, T> {
    using Result = NullType;
};

template<class T> struct make_pair<T, NullType> {
    using Result = NullType;
};

template<class Head, class Tail>
struct make_pair {
    using Result = Typelist<Head, Typelist<Tail, NullType>>;
    static_assert(TL::Length<Result>::value == 2, "");
};

template<class T1, class T2>
using make_pair_t = typename make_pair<T1, T2>::Result;
#endif

#if 0
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
    using Result = TL::Append_t<Item, Next>;
};
#endif
