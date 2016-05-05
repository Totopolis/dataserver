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

template<class T, size_t key_pos, bool search = where_::is_condition_SEARCH<T::cond>::value>
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

template<class T, size_t key_pos, bool search = where_::is_condition_SEARCH<T::cond>::value>
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

//---------------------------------------------

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

//----------------------------------------------------

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

//----------------------------------------------------

template<class T, class TList> struct sub_join;

template<> struct sub_join<NullType, NullType> {
    using Result = NullType;
};

template<class T> struct sub_join<NullType, T> {
    using Result = NullType;
};

template<class T> struct sub_join<T, NullType> {
    using Result = NullType;
};

template<class T, class U, class Head, class Tail>
struct sub_join<Typelist<T, U>, Typelist<Head, Tail>> {};

template<class T, class Head, class Tail>
struct sub_join<T, Typelist<Head, Tail>> {

    A_STATIC_ASSERT_NOT_TYPE(Head, NullType);
    static_assert(!IsTypelist<T>::value, "");

    struct key_type {
        using type_list = make_pair_t<T, Head>;
        static_assert(TL::Length<type_list>::value == 2, "");

#if SDL_DEBUG_QUERY
        static void trace(size_t & count){
            SDL_TRACE_QUERY("\nkey_type[", count++, "] = ");
            SDL_TRACE_QUERY("first = ", meta::short_name<T>());
            SDL_TRACE_QUERY("second = ", meta::short_name<Head>());
        }
#endif
    };
private:
    using Item = Typelist<key_type, NullType>;
    using Next = typename sub_join<T, Tail>::Result;
public:
    using Result = TL::Append_t<Item, Next>;

#if SDL_DEBUG_QUERY
private:
    static void trace(size_t &, identity<NullType>){}    
    template<class _Next>
    static void trace(size_t & count, identity<_Next>){
        sub_join<T, Tail>::trace(count);
    }
public:
    static void trace(size_t & count){
        SDL_TRACE_QUERY("\nsub_join[", count, "] = ", TL::Length<Result>::value);
        SDL_TRACE_QUERY("Item = ", TL::Length<Item>::value);
        key_type::trace(count);
        SDL_TRACE_QUERY("Next = ", TL::Length<Next>::value);
        trace(count, identity<Next>{});
    }
#endif
};

//----------------------------------------------------

template<class TList, class T> struct join_key;

template <class T> struct join_key<NullType, T> {
    using Result = NullType;
public:
    static void trace(size_t &){}
};

template <class Head, class Tail, class T>
struct join_key<Typelist<Head, Tail>, T> {
private:
    using join_head = typename sub_join<Head, T>::Result; 
    static_assert(TL::Length<join_head>::value == TL::Length<T>::value, "join_key");
public:
    using Result = TL::Append_t<join_head, typename join_key<Tail, T>::Result>;

#if SDL_DEBUG_QUERY
private:
    static void trace(size_t &, identity<NullType>){}   
    template<class _head> static void trace(size_t & count, identity<_head>){
        SDL_TRACE_QUERY("\njoin_key[", count, "]:");
        sub_join<Head, T>::trace(count);
    }
public:
    static void trace(size_t & count){
        trace(count, identity<join_head>{});
        join_key<Tail, T>::trace(count);
    }
#endif
};

template<class T1, class T2>
using join_key_t = typename join_key<T1, T2>::Result;

//----------------------------------------------------

struct SEARCH_KEY_BASE {
protected:
    template <class T, operator_ OP> using key_OR_0 = select_key<T, OP, 0, operator_::OR>;
    template <class T, operator_ OP> using key_AND_0 = select_key<T, OP, 0, operator_::AND>;
    template <class T, operator_ OP> using key_AND_1 = select_key<T, OP, 1, operator_::AND>;
    template <class T, operator_ OP> using no_key_0 = select_no_key<T, OP, 0, operator_::OR>;
};

template<class sub_expr_type>
struct SEARCH_KEY : SEARCH_KEY_BASE 
{
private:
    using key_OR_0_ = typename search_key<key_OR_0,
        typename sub_expr_type::type_list,
        typename sub_expr_type::oper_list,
        0
    >::Result;

    using key_AND_1_ = typename search_key<key_AND_1,
        typename sub_expr_type::type_list,
        typename sub_expr_type::oper_list,
        0
    >::Result;

    using no_key_0_ = typename search_key<no_key_0,
        typename sub_expr_type::type_list,
        typename sub_expr_type::oper_list,
        0
    >::Result;

    using key_AND_0_ = typename search_key<key_AND_0,
        typename sub_expr_type::type_list,
        typename sub_expr_type::oper_list,
        0
    >::Result;

public:
    using Result = Select_t<TL::Length<key_AND_1_>::value, join_key_t<key_OR_0_, key_AND_1_>, key_OR_0_>;
    
#if SDL_DEBUG_QUERY
    static void trace() {
        SDL_TRACE_QUERY("no_key_0 = ", TL::Length<no_key_0_>::value);
        SDL_TRACE_QUERY("no_key_0 = ", meta::short_name<no_key_0_>());
        SDL_TRACE_QUERY("key_AND_0 = ", TL::Length<key_AND_0_>::value);
        SDL_TRACE_QUERY("key_AND_0 =", meta::short_name<key_AND_0_>());

        SDL_TRACE_QUERY("\nSEARCH_KEY: [key_OR_0 = ",
            TL::Length<key_OR_0_>::value, "] join [key_AND_1 = ",
            TL::Length<key_AND_1_>::value, "] => ",
            TL::Length<Result>::value
            );
        size_t count = 0;
        join_key<key_OR_0_, key_AND_1_>::trace(count);
    }
#endif
    static_assert((0 == TL::Length<key_AND_1_>::value) || 
        (TL::Length<Result>::value == TL::Length<key_OR_0_>::value * TL::Length<key_AND_1_>::value), "");
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
#if 1
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

template<class _search_where_list, bool is_limit>
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
struct SELECT_TOP_ {
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

    static size_t value(sub_expr_type const & expr, identity<NullType>) {
        return 0;
    }
    template<class T>
    static size_t value(sub_expr_type const & expr, identity<T>) {
        static_assert(TL::Length<T>::value == 1, "");
        enum { offset = T::Head::offset };    
        A_STATIC_ASSERT_TYPE(size_t, where_::TOP::value_type);
        return expr.get(Size2Type<offset>())->value;
    }
public:
    static size_t value(sub_expr_type const & expr) {
        static_assert(TL::Length<TOP_2>::value < 2, "TOP duplicate");
        return value(expr, identity<TOP_2>{});
    }
};

template<class sub_expr_type>
inline size_t SELECT_TOP(sub_expr_type const & expr) {
    return SELECT_TOP_<sub_expr_type>::value(expr);
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

namespace select_order_ {

    template<class T> struct get_column { // T = SEARCH_WHERE<where_::ORDER_BY>
        using type = typename T::col;
    };
    template<> struct get_column<NullType> {
        using type = NullType;
    };
    template<class T> struct get_sortorder { // T = SEARCH_WHERE<where_::ORDER_BY>
        static const sortorder order = T::type::value;
    };
    template<> struct get_sortorder<NullType> {
        static const sortorder order = sortorder::NONE;
    };
    template<class col> struct is_cluster {
        enum { value = col::PK && (0 == col::key_pos) };
    };
    template<> struct is_cluster<NullType> {
        enum { value = false };
    };

} // select_order_

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

    using ORDER_LAST = typename TL::TypeLast<ORDER_2>::Result;
    using COL_LAST = typename select_order_::get_column<ORDER_LAST>::type;

    enum { last_is_cluster = select_order_::is_cluster<COL_LAST>::value };
    static const sortorder last_order = select_order_::get_sortorder<ORDER_LAST>::order;

    // if last sort is by cluster index ignore other ORDER_BY
    // Note. last sort is by cluster index in ASC order may skip sorting at all (depends how records are selected)
    using ORDER_3 = Select_t<last_is_cluster, Typelist<ORDER_LAST, NullType>, ORDER_2>; 
public:
    using Result = Select_t<last_is_cluster && (last_order == sortorder::ASC), NullType, ORDER_3>;
};

//--------------------------------------------------------------

template<class sub_expr_type>
struct SELECT_SEARCH_TYPE {
private:
    using T = search_condition<
        where_::is_condition_SEARCH,
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

template<class col, sortorder order, bool stable_sort> 
struct record_sort
{
    template<class record_range>
    static void sort(record_range & range) {
        static_assert(!stable_sort, "");
        using record = typename record_range::value_type;
        std::sort(range.begin(), range.end(), [](record const & x, record const & y){
            return meta::col_less<col, order>::less(
                x.val(identity<col>{}), 
                y.val(identity<col>{}));
        });
    }
};

template<class col, sortorder order> 
struct record_sort<col, order, true>
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

template<class TList, bool stable_sort> struct SORT_RECORD_RANGE;
template<bool stable_sort> struct SORT_RECORD_RANGE<NullType, stable_sort>
{
    template<class record_range>
    static void sort(record_range &){}
};

template<class Head, bool stable_sort>
struct SORT_RECORD_RANGE<Typelist<Head, NullType>, stable_sort>
{
    template<class record_range>
    static void sort(record_range & range) {
        record_sort<typename Head::col, Head::type::value, stable_sort>::sort(range);
    }
};

template<class Head, class Tail, bool stable_sort>
struct SORT_RECORD_RANGE<Typelist<Head, Tail>, stable_sort>  // Head = SEARCH_WHERE<where_::ORDER_BY>
{
    template<class record_range>
    static void sort(record_range & range) {
        record_sort<typename Head::col, Head::type::value, stable_sort>::sort(range);
        SORT_RECORD_RANGE<Tail, true>::sort(range);
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

    SDL_TRACE_QUERY("\nVALUES:");
    if (1) {
        where_::trace_::trace_sub_expr(expr);
    }
    record_range result;

    using ORDER = typename SELECT_ORDER_TYPE<sub_expr_type>::Result;
    using SEARCH = typename SELECT_SEARCH_TYPE<sub_expr_type>::Result;

    static_assert(CHECK_INDEX<sub_expr_type>::value, "");
    static_assert(index_size <= 2, "TODO: SEARCH_KEY");

#if SDL_DEBUG_QUERY
    SEARCH_KEY<sub_expr_type>::trace();
#endif

    if (auto const limit = SELECT_TOP(expr))
        SCAN_TABLE<SEARCH, true>(limit).select(result, this, expr);
    else
        SCAN_TABLE<SEARCH, false>(limit).select(result, this, expr);
    
    SORT_RECORD_RANGE<ORDER, false>::sort(result);
    return result;
}

} // make
} // db
} // sdl

#endif // __SDL_SYSTEM_MAKETABLE_HPP__

