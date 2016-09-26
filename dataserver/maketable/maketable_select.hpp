// maketable_select.hpp
//
#pragma once
#ifndef __SDL_SYSTEM_MAKETABLE_SELECT_HPP__
#define __SDL_SYSTEM_MAKETABLE_SELECT_HPP__

#if SDL_DEBUG && defined(SDL_OS_WIN32)
#define SDL_TRACE_QUERY(...)    SDL_TRACE(__VA_ARGS__)
#define SDL_DEBUG_QUERY         1
#else
#define SDL_TRACE_QUERY(...)    ((void)0)
#define SDL_DEBUG_QUERY         0
#endif

namespace sdl { namespace db { namespace make {

namespace make_query_ {

using where_::operator_;
using where_::operator_t;
using where_::operator_list;
using where_::operator_name;

template<size_t i, class T, operator_ op> // T = where_::SEARCH
struct SEARCH_WHERE
{
    enum { offset = i };
    using type = T;
    using col = typename T::col;
    static const operator_ OP = op;
    static const condition cond = T::cond;
};

//--------------------------------------------------------------

template<class T, bool enabled = where_::has_index_hint<T::cond>::value>
struct index_hint;

template<class T>
struct index_hint<T, true> { // allow INDEX::IGNORE|AUTO for any col
    static_assert((T::hint != where_::INDEX::USE) || (T::col::PK || T::col::spatial_key), "INDEX::USE need primary key or spatial index");
    static_assert((T::hint != where_::INDEX::USE) || (T::col::key_pos == 0), "INDEX::USE need key_pos 0");
    static_assert((T::hint != where_::INDEX::USE) || (T::cond != condition::NOT), "INDEX::USE cannot be used with condition::NOT");
    static const where_::INDEX hint = T::hint;
};

template<class T>
struct index_hint<T, false> {
    static const where_::INDEX hint = where_::INDEX::AUTO;
};

//--------------------------------------------------------------

template<class T, size_t key_pos, bool enabled = where_::is_condition_index<T::cond>::value>
struct use_index; // cluster_index

template<class T, size_t key_pos>
struct use_index<T, key_pos, true> {
private:
    static_assert(T::hint == index_hint<T>::hint, "use_index");
public:
    enum { value = T::col::PK && (T::col::key_pos == key_pos) && (T::hint != where_::INDEX::IGNORE) };
};

template<class T, size_t key_pos>
struct use_index<T, key_pos, false> {
public:
    enum { value = false };
};

//--------------------------------------------------------------

template<class T, bool enabled = where_::is_condition_spatial<T::cond>::value>
struct use_spatial_index;

template<class T>
struct use_spatial_index<T, true> {
private:
    enum { spatial_key = (T::col::key == meta::key_t::spatial_key) &&
        where_::enable_spatial_index<T, T::cond>::value };
    static_assert(T::hint == index_hint<T>::hint, "use_spatial_index");
    static_assert(T::col::key_pos == 0, "key_pos");
public:
    enum { value = spatial_key && (T::hint != where_::INDEX::IGNORE) };
};

template<class T>
struct use_spatial_index<T, false> {
public:
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
    enum { check = use_index<Head, 0>::value || use_spatial_index<Head, 0>::value };
    static_assert(!check || (index_hint<Head>::hint == where_::INDEX::AUTO), "check_index");
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
    enum { spatial = where_::is_condition_spatial<T::cond>::value
        && where_::enable_spatial_index<T, T::cond>::value };
    enum { search = where_::is_condition_search<T::cond>::value && !spatial };
    enum { temp = search && !use_index<T, key_pos>::value };
public:
    enum { value = temp && (OP == key_op) };
};

template <class T, operator_ OP, operator_ key_op> // T = where_::SEARCH
struct select_spatial {
private:
    enum { temp = use_spatial_index<T>::value };
public:
    enum { value = temp && (OP == key_op) };
};

//--------------------------------------------------------------

template <class T, operator_ OP, operator_ key_op> // T = where_::SEARCH
struct select_lambda {
private:
    enum { temp = where_::is_condition_lambda<T::cond>::value };
public:
    enum { value = temp && (OP == key_op) };
};

template <class T, operator_ OP> // T = where_::SEARCH
struct select_OR {
private:
    enum { temp = where_::is_condition_search<T::cond>::value };
public:
    enum { value = temp && (OP == operator_::OR) };
};

template <class T, operator_ OP> // T = where_::SEARCH
struct select_AND {
private:
    enum { temp = where_::is_condition_search<T::cond>::value };
public:
    enum { value = temp && (OP == operator_::AND) };
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
    template <class T, operator_ OP> using _spatial_OR = select_spatial<T, OP, operator_::OR>;
    template <class T, operator_ OP> using _spatial_AND = select_spatial<T, OP, operator_::AND>;
};

template<class sub_expr_type>
struct SEARCH_KEY : SEARCH_KEY_BASE 
{
    using search_OR = typename search_key<select_OR,
        typename sub_expr_type::type_list,
        typename sub_expr_type::oper_list,
        0
    >::Result;

    using search_AND = typename search_key<select_AND,
        typename sub_expr_type::type_list,
        typename sub_expr_type::oper_list,
        0
    >::Result;

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

    using spatial_OR = typename search_key<_spatial_OR,
        typename sub_expr_type::type_list,
        typename sub_expr_type::oper_list,
        0
    >::Result;

    using spatial_AND = typename search_key<_spatial_AND,
        typename sub_expr_type::type_list,
        typename sub_expr_type::oper_list,
        0
    >::Result;

    static_assert(TL::Length<search_OR>::value, "SEARCH_KEY: empty OR clause");
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
    using search_where = NullType;
};

template <template <condition> class compare, class Head, class Tail, operator_ OP, class NextOP, size_t i>
struct search_condition<compare, Typelist<Head, Tail>, operator_list<OP, NextOP>, i> {
private:
    enum { flag = compare<Head::cond>::value };
    using indx_i = Select_t<flag, Typelist<Int2Type<i>, NullType>, NullType>;
    using type_i = Select_t<flag, Typelist<Head, NullType>, NullType>;
    using oper_i = Select_t<flag, Typelist<operator_t<OP>, NullType>, NullType>;
    using where_i = Select_t<flag, Typelist<SEARCH_WHERE<i, Head, OP>,  NullType>, NullType>;
public:
    using Index = TL::Append_t<indx_i, typename search_condition<compare, Tail, NextOP, i + 1>::Index>;
    using Types = TL::Append_t<type_i, typename search_condition<compare, Tail, NextOP, i + 1>::Types>; 
    using OList = TL::Append_t<oper_i, typename search_condition<compare, Tail, NextOP, i + 1>::OList>;
    using search_where = TL::Append_t<where_i, typename search_condition<compare, Tail, NextOP, i + 1>::search_where>;    
};

//--------------------------------------------------------------
#if 0 // gcc compiler error
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

struct DISTANCE : is_static {
    static bool compare(Meters const d1, Meters const d2, where_::compare_t<where_::compare::equal>) {
        return d1.value() == d2.value();
    }
    static bool compare(Meters const d1, Meters const d2, where_::compare_t<where_::compare::less>) {
        return d1.value() < d2.value();
    }
    static bool compare(Meters const d1, Meters const d2, where_::compare_t<where_::compare::less_eq>) {
        return d1.value() <= d2.value();
    }
    static bool compare(Meters const d1, Meters const d2, where_::compare_t<where_::compare::greater>) {
        return d1.value() > d2.value();
    }
    static bool compare(Meters const d1, Meters const d2, where_::compare_t<where_::compare::greater_eq>) { 
        return d1.value() >= d2.value();
    }
};

template<class T>       // T = SEARCH_WHERE 
struct RECORD_SELECT {

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
private:
    template<class record, class expr_type> static bool select(record const & p, expr_type const * const expr, condition_t<condition::WHERE>);
    template<class record, class expr_type> static bool select(record const & p, expr_type const * const expr, condition_t<condition::IN>);
    template<class record, class expr_type> static bool select(record const & p, expr_type const * const expr, condition_t<condition::NOT>);
    template<class record, class expr_type> static bool select(record const & p, expr_type const * const expr, condition_t<condition::LESS>);
    template<class record, class expr_type> static bool select(record const & p, expr_type const * const expr, condition_t<condition::GREATER>);
    template<class record, class expr_type> static bool select(record const & p, expr_type const * const expr, condition_t<condition::LESS_EQ>);
    template<class record, class expr_type> static bool select(record const & p, expr_type const * const expr, condition_t<condition::GREATER_EQ>);
    template<class record, class expr_type> static bool select(record const & p, expr_type const * const expr, condition_t<condition::BETWEEN>);
    template<class record, class expr_type> static bool select(record const & p, expr_type const * const expr, condition_t<condition::lambda>);
    template<class record, class expr_type> static bool select(record const & p, expr_type const * const expr, condition_t<condition::STContains>);
    template<class record, class expr_type> static bool select(record const & p, expr_type const * const expr, condition_t<condition::STIntersects>);
    template<class record, class expr_type> static bool select(record const & p, expr_type const * const expr, condition_t<condition::STDistance>);
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

template<class T>
template<class record, class expr_type> inline
bool RECORD_SELECT<T>::select(record const & p, expr_type const * const expr, condition_t<condition::STContains>) {
    static_assert(T::col::type == scalartype::t_geography, "STContains need t_geography");
    return p.val(identity<typename T::col>{}).STContains(expr->value.values);
}

template<class T>
template<class record, class expr_type> inline
bool RECORD_SELECT<T>::select(record const & p, expr_type const * const expr, condition_t<condition::STIntersects>) {
    static_assert(T::col::type == scalartype::t_geography, "STIntersects need t_geography");
    return p.val(identity<typename T::col>{}).STIntersects(expr->value.values);
}

template<class T>
template<class record, class expr_type> inline
bool RECORD_SELECT<T>::select(record const & p, expr_type const * const expr, condition_t<condition::STDistance>) {
    static_assert(T::col::type == scalartype::t_geography, "STDistance need t_geography");
    return DISTANCE::compare(
        p.val(identity<typename T::col>{}).STDistance(expr->value.values.first), 
        expr->value.values.second, where_::compare_t<T::type::comp>());
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

template<class sub_expr_type>
struct SELECT_TOP_TYPE {
private:
    using TOP_1 = search_condition<
        where_::is_condition_top,
        typename sub_expr_type::type_list,
        typename sub_expr_type::oper_list,
        0>;
    using TOP_2 = typename TOP_1::search_where;
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
    using ORDER_2 = typename ORDER_1::search_where;
    
    enum { check = CHECK_ORDER<ORDER_2>::value };
    static_assert(check, "SELECT_ORDER_TYPE");

    using temp = typename order_cluster<ORDER_2, sortorder::ASC>::Result;
    enum { remove = (TL::IndexOf<ORDER_2, temp>::value == 0) && (TL::Length<ORDER_2>::value == 1) };

    using ORDER_3 = Select_t<remove, NullType, ORDER_2>;
public:
    using Result = ORDER_3;
};

//--------------------------------------------------------------
#if 0 // gcc compiler error
template<class sub_expr_type>
struct SELECT_SEARCH_TYPE {
private:
    using T = search_condition<
        where_::is_condition_search,
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
#endif

template<class sub_expr_type>
struct SELECT_SEARCH_TYPE {
private:
    using T = search_condition<
        where_::is_condition_search,
        typename sub_expr_type::type_list,
        typename sub_expr_type::oper_list,
        0>;
public:
    using Result = typename T::search_where;
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

template<class sub_expr_type>
struct IS_SEEK_TABLE {
private:
    using KEYS = SEARCH_KEY<sub_expr_type>;    
    enum { key_OR_0 = TL::Length<typename KEYS::key_OR_0>::value };
    enum { key_AND_0 = TL::Length<typename KEYS::key_AND_0>::value };
    enum { no_key_OR_0 = TL::Length<typename KEYS::no_key_OR_0>::value };
    enum { no_key_AND_0 = TL::Length<typename KEYS::no_key_AND_0>::value };
    enum { spatial_OR = TL::Length<typename KEYS::spatial_OR>::value };
    enum { spatial_AND = TL::Length<typename KEYS::spatial_AND>::value };
    enum { lambda_OR = TL::Length<typename KEYS::lambda_OR>::value };
public:
    enum { use_index = !lambda_OR && (key_AND_0 || (key_OR_0 && !no_key_OR_0)) };
    enum { spatial_index = !use_index && !lambda_OR && (spatial_AND || (spatial_OR && !no_key_OR_0)) };

#if SDL_DEBUG_QUERY
    static void trace(){
        SDL_TRACE("key_OR_0 = ", key_OR_0);
        SDL_TRACE("key_AND_0 = ", key_AND_0);
        SDL_TRACE("no_key_OR_0 = ", no_key_OR_0);
        SDL_TRACE("no_key_AND_0 = ", no_key_AND_0);
        SDL_TRACE("spatial_OR = ", spatial_OR);
        SDL_TRACE("spatial_AND = ", spatial_AND);
        SDL_TRACE("lambda_OR = ", lambda_OR);
        SDL_TRACE("use_index = ", use_index);
        SDL_TRACE("spatial_index = ", spatial_index);
    }
#endif
};

//--------------------------------------------------------------

template<class query_type, class sub_expr_type, bool is_limit>
class SCAN_TABLE final : noncopyable {

    using record_range = typename query_type::record_range;
    using record = typename query_type::record;

    using SEARCH = typename SELECT_SEARCH_TYPE<sub_expr_type>::Result;
    using search_AND = search_operator_t<operator_::AND, SEARCH>;
    using search_OR = search_operator_t<operator_::OR, SEARCH>;
    static_assert(TL::Length<search_OR>::value, "empty OR");

    static bool has_limit(std::false_type) {
        return false;
    }
    bool has_limit(std::true_type) const {
        return m_limit <= m_result.size();
    }
    bool is_select(record const & p) const {
        return
            SELECT_OR<search_OR, true>::select(p, this->m_expr) &&    // any of 
            SELECT_AND<search_AND, true>::select(p, this->m_expr);    // must be
    }
public:
    record_range &          m_result;
    query_type &            m_query;
    sub_expr_type const &   m_expr;
    const size_t            m_limit;

    SCAN_TABLE(record_range & result, query_type & query, sub_expr_type const & expr, size_t lim)
        : m_result(result)
        , m_query(query)
        , m_expr(expr)
        , m_limit(lim)
    {
        SDL_ASSERT(is_limit == (m_limit > 0));
        static_assert(!IS_SEEK_TABLE<sub_expr_type>::use_index, "SCAN_TABLE");
    }
    void select();
};

template<class query_type, class sub_expr_type, bool is_limit>
void SCAN_TABLE< query_type, sub_expr_type, is_limit>::select() {
    m_query.scan_if([this](record const p){
        if (is_select(p)) {
            m_result.push_back(p);
            if (has_limit(bool_constant<is_limit>{}))
                return false;
        }
        return true;
    });
}

} // make_query_

//////////////////////////////////////////////////////////////////////////////////////////

template<class this_table, class _record> 
class make_query<this_table, _record>::seek_table final : is_static
{
    using query_type = make_query<this_table, _record>;
    using record = typename query_type::record;
    using col_type = typename query_type::T0_col;
    using value_type = typename query_type::T0_type;

    static_assert(query_type::index_size, "");
    static_assert(col_type::order == sortorder::ASC, "seek_table need sortorder::ASC"); //FIXME: sortorder::DESC...

    enum { is_composite = query_type::index_size > 1 };

    template<class fun_type, class T> static break_or_continue scan_or_find(query_type &, value_type const &, fun_type &&, identity<T>, std::false_type);
    template<class fun_type, class T> static break_or_continue scan_or_find(query_type &, value_type const &, fun_type &&, identity<T>, std::true_type);
    template<class fun_type, class T> static break_or_continue scan_where(query_type &, value_type const &, fun_type &&, identity<T>);

    struct is_equal {
        static bool apply(record const & p, value_type const & v) {
            return meta::is_equal<col_type>::equal(p.val(identity<col_type>{}), v);
        }
    };
    template<sortorder ord>
    struct is_less {
        static bool apply(record const & p, value_type const & v) {
            return meta::col_less<col_type, ord>::less(p.val(identity<col_type>{}), v);
        }
    };
    template<sortorder ord>
    struct is_less_eq {
        static bool apply(record const & p, value_type const & v) {
            return is_less<ord>::apply(p, v) || is_equal::apply(p, v);
        }
    };

    template<class expr_type, class fun_type, class less_type> static
    break_or_continue scan_less(query_type &, expr_type const *, fun_type &&, identity<less_type>);
public:
    // T = make_query_::SEARCH_WHERE
    template<class expr_type, class fun_type, class T> static break_or_continue scan_if(query_type &, expr_type const *, fun_type &&, identity<T>, condition_t<condition::WHERE>);
    template<class expr_type, class fun_type, class T> static break_or_continue scan_if(query_type &, expr_type const *, fun_type &&, identity<T>, condition_t<condition::IN>);
    template<class expr_type, class fun_type, class T> static break_or_continue scan_if(query_type &, expr_type const *, fun_type &&, identity<T>, condition_t<condition::LESS>);
    template<class expr_type, class fun_type, class T> static break_or_continue scan_if(query_type &, expr_type const *, fun_type &&, identity<T>, condition_t<condition::GREATER>);
    template<class expr_type, class fun_type, class T> static break_or_continue scan_if(query_type &, expr_type const *, fun_type &&, identity<T>, condition_t<condition::LESS_EQ>);
    template<class expr_type, class fun_type, class T> static break_or_continue scan_if(query_type &, expr_type const *, fun_type &&, identity<T>, condition_t<condition::GREATER_EQ>);
    template<class expr_type, class fun_type, class T> static break_or_continue scan_if(query_type &, expr_type const *, fun_type &&, identity<T>, condition_t<condition::BETWEEN>);
private:
    template<class expr_type, class fun_type, class T> static void scan_if(query_type &, expr_type const *, fun_type &&, identity<T>, condition_t<condition::NOT>) {} // not called
};

template<class this_table, class _record>
template<class fun_type, class T> inline break_or_continue
make_query<this_table, _record>::seek_table::scan_or_find(query_type & query, value_type const & v, fun_type && fun, identity<T>, std::false_type) {
    static_assert(!is_composite, "");
    if (auto found = query.find_with_index(query_type::make_key(v))) {
        return fun(found);
    }
    return bc::continue_;
}

template<class this_table, class _record>
template<class fun_type, class T> break_or_continue
make_query<this_table, _record>::seek_table::scan_or_find(query_type & query, value_type const & value, fun_type && fun, identity<T>, std::true_type) {
    static_assert(is_composite, "");
    break_or_continue result = bc::continue_;
    auto const found = query.lower_bound(value);
    if (found.second) {
        A_STATIC_CHECK_TYPE(bool, found.second);
        query.scan_next(found.first, [&result, &value, &fun](record const & p){
            if (is_equal::apply(p, value)) {
                return bc::continue_ == (result = fun(p));
            }
            return false;
        });
    }
    return result;
}

template<class this_table, class _record>
template<class fun_type, class T> inline break_or_continue
make_query<this_table, _record>::seek_table::scan_where(query_type & query, value_type const & value, fun_type && fun, identity<T>) {
    return scan_or_find(query, value, fun, identity<T>{}, bool_constant<is_composite>{});
}

template<class this_table, class _record> template<class expr_type, class fun_type, class T> inline break_or_continue
make_query<this_table, _record>::seek_table::scan_if(query_type & query, expr_type const * const expr, fun_type && fun, identity<T>, condition_t<condition::WHERE>) {    
    return scan_where(query, expr->value.values, fun, identity<T>{});
}

template<class this_table, class _record> template<class expr_type, class fun_type, class T> break_or_continue
make_query<this_table, _record>::seek_table::scan_if(query_type & query, expr_type const * const expr, fun_type && fun, identity<T>, condition_t<condition::IN>) {
    for (auto & v : expr->value.values) {
        if (bc::break_ == scan_where(query, v, fun, identity<T>{})) {
            return bc::break_;
        }
    }
    return bc::continue_;
}

template<class this_table, class _record> 
template<class expr_type, class fun_type, class less_type> break_or_continue
make_query<this_table, _record>::seek_table::scan_less(query_type & query, expr_type const * const expr, fun_type && fun, identity<less_type>)
{
    break_or_continue result = bc::continue_;
    query.scan_if([&result, &fun, expr](record const & p){
        if (less_type::apply(p, expr->value.values)) {
            return bc::continue_ == (result = fun(p));
        }
        return false;
    });
    return result;
}

template<class this_table, class _record> 
template<class expr_type, class fun_type, class T> inline break_or_continue
make_query<this_table, _record>::seek_table::scan_if(query_type & query, expr_type const * const expr, fun_type && fun, identity<T>, condition_t<condition::LESS>)
{
    return scan_less(query, expr, fun, identity<is_less<sortorder::ASC>>{});
}

template<class this_table, class _record> 
template<class expr_type, class fun_type, class T> inline break_or_continue
make_query<this_table, _record>::seek_table::scan_if(query_type & query, expr_type const * const expr, fun_type && fun, identity<T>, condition_t<condition::LESS_EQ>)
{
    return scan_less(query, expr, fun, identity<is_less_eq<sortorder::ASC>>{});
}

template<class this_table, class _record> 
template<class expr_type, class fun_type, class T> break_or_continue
make_query<this_table, _record>::seek_table::scan_if(query_type & query, expr_type const * const expr, fun_type && fun, identity<T>, condition_t<condition::GREATER>) {
    break_or_continue result = bc::continue_;
    auto found = query.lower_bound(expr->value.values);
    if (found.first.page) {
        if (found.second) { // skip equal values
            found.first = query.scan_next(found.first, [expr](record const & p){
                return is_equal::apply(p, expr->value.values);
            });
        }
        if (found.first.page) {
            SDL_ASSERT(is_less<sortorder::DESC>::apply(query.get_record(found.first), expr->value.values));
            query.scan_next(found.first, [&result, &fun](record const & p){
                return bc::continue_ == (result = fun(p));
            });
        }
    }
    return result;
}

template<class this_table, class _record> 
template<class expr_type, class fun_type, class T> break_or_continue
make_query<this_table, _record>::seek_table::scan_if(query_type & query, expr_type const * const expr, fun_type && fun, identity<T>, condition_t<condition::GREATER_EQ>) {
    break_or_continue result = bc::continue_;
    auto found = query.lower_bound(expr->value.values);
    if (found.first.page) {
        SDL_ASSERT(is_less_eq<sortorder::DESC>::apply(query.get_record(found.first), expr->value.values));
        query.scan_next(found.first, [&result, &fun](record const & p){
            return bc::continue_ == (result = fun(p));
        });
    }
    return result;
}

template<class this_table, class _record> 
template<class expr_type, class fun_type, class T> break_or_continue
make_query<this_table, _record>::seek_table::scan_if(query_type & query, expr_type const * const expr, fun_type && fun, identity<T>, condition_t<condition::BETWEEN>) {
    break_or_continue result = bc::continue_;
    auto found = query.lower_bound(expr->value.values.first);
    if (found.first.page) {
        query.scan_next(found.first, [&result, expr, &fun](record const & p){
            if (is_less_eq<sortorder::ASC>::apply(p, expr->value.values.second)) {
                return bc::continue_ == (result = fun(p));
            }
            return false;
        });
    }
    return result;
}

//////////////////////////////////////////////////////////////////////////////////////////

template<class T, class key_type>
bool binary_insertion(T & result, key_type && unique_key) {
    if (!result.empty()) {
        const auto pos = std::lower_bound(result.begin(), result.end(), unique_key);
        if (pos != result.end()) {
            if (*pos == unique_key) {
                return false;
            }
            result.insert(pos, std::forward<key_type>(unique_key));
            return true;
        }
    }
    result.push_back(std::forward<key_type>(unique_key)); 
    return true;
}

template<class this_table, class _record> 
class make_query<this_table, _record>::seek_spatial final : is_static
{
    using query_type = make_query<this_table, _record>;
    using record = typename query_type::record;
    using pk0_type = typename query_type::T0_type;
#if 0
    using vector_pk0 = std::vector<pk0_type>;
#else
    using vector_pk0 = interval_set<pk0_type>;
#endif
    template<class fun_type>
    class for_point_fun : noncopyable {
        query_type & m_query;
        fun_type & m_fun;
        vector_pk0 m_pk0; // already processed records
    public:
        for_point_fun(query_type & q, fun_type & p): m_query(q), m_fun(p){}
#if 0
        template<class spatial_page_row>
        break_or_continue operator()(spatial_page_row const * const p) {
            A_STATIC_ASSERT_TYPE(spatial_page_row, typename query_type::spatial_page_row);
            A_STATIC_CHECK_TYPE(T0_type, p->data.pk0);
            if (binary_insertion(m_pk0, p->data.pk0)) {
                if (auto found = m_query.find_with_index(query_type::make_key(p->data.pk0))) { // found record
                    A_STATIC_CHECK_TYPE(record, found);
                    return m_fun(found);
                }
                SDL_ASSERT(!"bad primary key");
                return bc::break_;
            }
            return bc::continue_;
        }
#else
        template<class spatial_page_row>
        break_or_continue operator()(spatial_page_row const * const p) {
            A_STATIC_ASSERT_TYPE(spatial_page_row, typename query_type::spatial_page_row);
            A_STATIC_CHECK_TYPE(T0_type, p->data.pk0);
            if (m_pk0.insert(p->data.pk0)) {
                if (auto found = m_query.find_with_index(query_type::make_key(p->data.pk0))) { // found record
                    A_STATIC_CHECK_TYPE(record, found);
                    return m_fun(found);
                }
                SDL_ASSERT(!"bad primary key");
                return bc::break_;
            }
            return bc::continue_;
        }
#endif
    };
public:
    // T = make_query_::SEARCH_WHERE
    template<class expr_type, class fun_type, class T> static break_or_continue scan_if(query_type &, expr_type const *, fun_type &&, identity<T>, condition_t<condition::STContains>);
    template<class expr_type, class fun_type, class T> static break_or_continue scan_if(query_type &, expr_type const *, fun_type &&, identity<T>, condition_t<condition::STIntersects>);
    template<class expr_type, class fun_type, class T> static break_or_continue scan_if(query_type &, expr_type const *, fun_type &&, identity<T>, condition_t<condition::STDistance>);
};

template<class this_table, class _record> template<class expr_type, class fun_type, class T> break_or_continue
make_query<this_table, _record>::seek_spatial::scan_if(query_type & query, expr_type const * const expr, fun_type && fun, identity<T>, condition_t<condition::STContains>) {
    A_STATIC_CHECK_TYPE(spatial_point, expr->value.values);
    static_assert(T::col::type == scalartype::t_geography, "STContains need t_geography");
    if (auto tree = query.get_spatial_tree()) {
        auto select_fun = [expr, &fun](record const p) {
            if (p.val(identity<typename T::col>{}).STContains(expr->value.values)) {
                return fun(p);
            }
            return bc::continue_;
        };
        return tree->for_point(expr->value.values, for_point_fun<decltype(select_fun)>(query, select_fun));
    }
    SDL_ASSERT(0);
    return bc::break_;
}

template<class this_table, class _record> template<class expr_type, class fun_type, class T> break_or_continue
make_query<this_table, _record>::seek_spatial::scan_if(query_type & query, expr_type const * const expr, fun_type && fun, identity<T>, condition_t<condition::STIntersects>) {
    A_STATIC_CHECK_TYPE(spatial_rect, expr->value.values);
    static_assert(T::col::type == scalartype::t_geography, "STIntersects need t_geography");
    if (auto tree = query.get_spatial_tree()) {
        auto select_fun = [expr, &fun](record const p) {
            if (p.val(identity<typename T::col>{}).STIntersects(expr->value.values)) {
                return fun(p);
            }
            return bc::continue_;
        };
        return tree->for_rect(expr->value.values, for_point_fun<decltype(select_fun)>(query, select_fun));
    }
    SDL_ASSERT(0);
    return bc::break_;
}

template<class this_table, class _record> template<class expr_type, class fun_type, class T> break_or_continue
make_query<this_table, _record>::seek_spatial::scan_if(query_type & query, expr_type const * const expr, fun_type && fun, identity<T>, condition_t<condition::STDistance>) {
    static_assert(T::col::type == scalartype::t_geography, "STDistance need t_geography");
    A_STATIC_CHECK_TYPE(spatial_point, expr->value.values.first);
    A_STATIC_CHECK_TYPE(Meters, expr->value.values.second);
    if (auto tree = query.get_spatial_tree()) {
        auto select_fun = [expr, &fun](record const p) {
            if (make_query_::DISTANCE::compare(
                    p.val(identity<typename T::col>{}).STDistance(expr->value.values.first), 
                    expr->value.values.second,
                    where_::compare_t<T::type::comp>()))
            {
                return fun(p);
            }
            return bc::continue_;
        };
        static_assert(where_::for_range<T::type::comp>::value, "STDistance use_for_range");
        return tree->for_range(
            expr->value.values.first, 
            expr->value.values.second,
            for_point_fun<decltype(select_fun)>(query, select_fun));
    }
    SDL_ASSERT(0);
    return bc::break_;
}


//////////////////////////////////////////////////////////////////////////////////////////

namespace make_query_ {

template<class query_type, class sub_expr_type, bool is_limit>
class SEEK_TABLE final : noncopyable {

    using this_type = SEEK_TABLE;
    using record_range = typename query_type::record_range;
    using record = typename query_type::record;
    using key_type = typename query_type::key_type;

    using KEYS = SEARCH_KEY<sub_expr_type>;
    using key_OR_0 = typename KEYS::key_OR_0;
    using key_AND_0 = typename KEYS::key_AND_0;
    static_assert(TL::Length<key_OR_0>::value || TL::Length<key_AND_0>::value, "SEEK_TABLE");

    template<class expr_type, class T>
    bool seek_with_index(expr_type const * const expr, identity<T>);
    
    struct seek_with_index_t {        
        this_type * const m_this;
        explicit seek_with_index_t(this_type * p) : m_this(p){}
        template<class T> 
        bool operator()(identity<T>) const { // T = SEARCH_WHERE
            return m_this->seek_with_index(m_this->m_expr.get(Size2Type<T::offset>()), identity<T>{});
        }
    };
    static bool has_limit(std::false_type) {
        return false;
    }
    bool has_limit(std::true_type) const {
        return m_limit <= m_result.size();
    }
    bool is_select(record const & p, operator_t<operator_::OR>) const;
    bool is_select(record const & p, operator_t<operator_::AND>) const;

    break_or_continue push_unique(record const &);
public:
    record_range &          m_result;
    query_type &            m_query;
    sub_expr_type const &   m_expr;
    const size_t            m_limit;

    SEEK_TABLE(record_range & result, query_type & query, sub_expr_type const & expr, size_t const lim)
        : m_result(result)
        , m_query(query)
        , m_expr(expr)
        , m_limit(lim)
    {
        SDL_ASSERT(is_limit == (m_limit > 0));
        static_assert(IS_SEEK_TABLE<sub_expr_type>::use_index, "SEEK_TABLE");
    }
    void select();
};

template<class query_type, class sub_expr_type, bool is_limit> inline
bool SEEK_TABLE<query_type, sub_expr_type, is_limit>::is_select(record const & p, operator_t<operator_::OR>) const
{
    return SELECT_AND<typename KEYS::search_AND, true>::select(p, m_expr); // must be
}

template<class query_type, class sub_expr_type, bool is_limit> inline
bool SEEK_TABLE<query_type, sub_expr_type, is_limit>::is_select(record const & p, operator_t<operator_::AND>) const
{
    return 
        SELECT_OR<typename KEYS::search_OR, true>::select(p, m_expr) &&     // any of 
        SELECT_AND<typename KEYS::no_key_AND_0, true>::select(p, m_expr);   // must be
}

template<class query_type, class sub_expr_type, bool is_limit> inline break_or_continue
SEEK_TABLE<query_type, sub_expr_type, is_limit>::push_unique(record const & p)
{
    A_STATIC_ASSERT_NOT_TYPE(void, typename key_type::this_clustered);
    if (query_type::push_unique_key(m_result, p)) {
        return has_limit(bool_constant<is_limit>{}) ? bc::break_ : bc::continue_;
    }
    return bc::continue_;
}

template<class query_type, class sub_expr_type, bool is_limit>
template<class expr_type, class T> inline // T = SEARCH_WHERE
bool SEEK_TABLE<query_type, sub_expr_type, is_limit>::seek_with_index(expr_type const * const expr, identity<T>)
{
    return query_type::seek_table::scan_if(m_query, expr, [this](record const p) {
        if (is_select(p, operator_t<T::OP>{})) { // check other part of condition 
            return push_unique(p);
        }
        return bc::continue_;
    }, 
    identity<T>{}, condition_t<T::cond>{}) == bc::continue_;
}

template<class query_type, class sub_expr_type, bool is_limit> inline
void SEEK_TABLE<query_type, sub_expr_type, is_limit>::select()
{
    using keylist = Select_t<TL::IsEmpty<key_AND_0>::value, key_OR_0, key_AND_0>;
    meta::processor_if<keylist>::apply(seek_with_index_t(this));
}

//---------------------------------------------------------------------------------

template<class query_type, class sub_expr_type, bool is_limit>
class SEEK_SPATIAL final : noncopyable {

    using this_type = SEEK_SPATIAL;
    using record_range = typename query_type::record_range;
    using record = typename query_type::record;
    using key_type = typename query_type::key_type;

    using KEYS = SEARCH_KEY<sub_expr_type>;
    using spatial_OR = typename SEARCH_KEY<sub_expr_type>::spatial_OR;
    using spatial_AND = typename SEARCH_KEY<sub_expr_type>::spatial_AND;
    static_assert(TL::Length<spatial_OR>::value || TL::Length<spatial_AND>::value, "SEEK_SPATIAL");

    template<class expr_type, class T>
    bool seek_with_index(expr_type const * const expr, identity<T>);
    
    struct seek_with_index_t {        
        this_type * const m_this;
        explicit seek_with_index_t(this_type * p) : m_this(p){}
        template<class T> 
        bool operator()(identity<T>) const { // T = SEARCH_WHERE
            return m_this->seek_with_index(m_this->m_expr.get(Size2Type<T::offset>()), identity<T>{});
        }
    };
    static bool has_limit(std::false_type) {
        return false;
    }
    bool has_limit(std::true_type) const {
        return m_limit <= m_result.size();
    }
    bool is_select(record const & p, operator_t<operator_::OR>) const;
    bool is_select(record const & p, operator_t<operator_::AND>) const;

    break_or_continue push_unique(record const &);
public:
    record_range &          m_result;
    query_type &            m_query;
    sub_expr_type const &   m_expr;
    const size_t            m_limit;

    SEEK_SPATIAL(record_range & result, query_type & query, sub_expr_type const & expr, size_t const lim)
        : m_result(result)
        , m_query(query)
        , m_expr(expr)
        , m_limit(lim)
    {
        SDL_ASSERT(is_limit == (m_limit > 0));
        static_assert(IS_SEEK_TABLE<sub_expr_type>::spatial_index, "SEEK_SPATIAL");
    }
    void select();
};

template<class query_type, class sub_expr_type, bool is_limit> inline
bool SEEK_SPATIAL<query_type, sub_expr_type, is_limit>::is_select(record const & p, operator_t<operator_::OR>) const
{
    return SELECT_AND<typename KEYS::search_AND, true>::select(p, m_expr); // must be
}

template<class query_type, class sub_expr_type, bool is_limit> inline
bool SEEK_SPATIAL<query_type, sub_expr_type, is_limit>::is_select(record const & p, operator_t<operator_::AND>) const
{
    return 
        SELECT_OR<typename KEYS::search_OR, true>::select(p, m_expr) &&     // any of 
        SELECT_AND<typename KEYS::no_key_AND_0, true>::select(p, m_expr);   // must be
}

template<class query_type, class sub_expr_type, bool is_limit> inline break_or_continue
SEEK_SPATIAL<query_type, sub_expr_type, is_limit>::push_unique(record const & p)
{
    A_STATIC_ASSERT_NOT_TYPE(void, typename key_type::this_clustered);
    if (query_type::push_unique_key(m_result, p)) {
        return has_limit(bool_constant<is_limit>{}) ? bc::break_ : bc::continue_;
    }
    return bc::continue_;
}

template<class query_type, class sub_expr_type, bool is_limit>
template<class expr_type, class T> inline // T = SEARCH_WHERE
bool SEEK_SPATIAL<query_type, sub_expr_type, is_limit>::seek_with_index(expr_type const * const expr, identity<T>)
{
    return query_type::seek_spatial::scan_if(m_query, expr, [this](record const p) {
        if (is_select(p, operator_t<T::OP>{})) { // check other part of condition 
            return push_unique(p);
        }
        return bc::continue_;
    }, 
    identity<T>{}, condition_t<T::cond>{}) == bc::continue_;
}

template<class query_type, class sub_expr_type, bool is_limit> inline
void SEEK_SPATIAL<query_type, sub_expr_type, is_limit>::select()
{
    using keylist = Select_t<TL::IsEmpty<spatial_AND>::value, spatial_OR, spatial_AND>;
    meta::processor_if<keylist>::apply(seek_with_index_t(this));
}

//---------------------------------------------------------------------------------

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
public:
    template<class record_range, class query_type> static
    void select(record_range & result, query_type & query, sub_expr_type const & expr);
};

template<class sub_expr_type, class TOP>
template<class record_range, class query_type> inline
void SCAN_OR_SEEK<sub_expr_type, TOP>::select(record_range & result, query_type & query, sub_expr_type const & expr)
{
    using Scan = SCAN_TABLE<query_type, sub_expr_type, is_limit>;
    using Seek = SEEK_TABLE<query_type, sub_expr_type, is_limit>;
    using Spatial = SEEK_SPATIAL<query_type, sub_expr_type, is_limit>;
    using seek_sub_expr = IS_SEEK_TABLE<sub_expr_type>;
#if SDL_DEBUG_QUERY
    seek_sub_expr::trace();
#endif
    using Table = Select_t<seek_sub_expr::spatial_index, Spatial, 
                  Select_t<seek_sub_expr::use_index, Seek, Scan>>;
    Table(result, query, expr, limit(expr)).select();
}

//--------------------------------------------------------------

template<class sub_expr_type, class TOP, class ORDER>
struct QUERY_VALUES
{
    static_assert(TL::Length<TOP>::value == 1, "TOP");
    static_assert(TL::Length<ORDER>::value, "ORDER");

    template<class record_range, class query_type> static
    void select(record_range & result, query_type & query, sub_expr_type const & expr) {
#if 1 // slow but more SQL like
        SCAN_OR_SEEK<sub_expr_type>::select(result, query, expr);
        SORT_RECORD_RANGE<ORDER>::sort(result);
        result.resize(a_min(SELECT_TOP(expr), result.size()));
        result.shrink_to_fit();
#else
        SCAN_OR_SEEK<sub_expr_type, TOP>::select(result, query, expr);
        SORT_RECORD_RANGE<ORDER>::sort(result);
#endif
    }
};

template<class sub_expr_type>
struct QUERY_VALUES<sub_expr_type, NullType, NullType>
{
    template<class record_range, class query_type> static
    void select(record_range & result, query_type & query, sub_expr_type const & expr) {
       SCAN_OR_SEEK<sub_expr_type>::select(result, query, expr);
    }
};

template<class sub_expr_type, class TOP>
struct QUERY_VALUES<sub_expr_type, TOP, NullType>
{
    template<class record_range, class query_type> static
    void select(record_range & result, query_type & query, sub_expr_type const & expr) {
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
    void select(record_range & result, query_type & query, sub_expr_type const & expr) {
        SCAN_OR_SEEK<sub_expr_type>::select(result, query, expr);
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

#if SDL_DEBUG_QUERY
    SDL_TRACE_QUERY("\nVALUES:");
    where_::trace_::trace_sub_expr(expr);
#endif
    using TOP = typename SELECT_TOP_TYPE<sub_expr_type>::Result;
    using ORDER = typename SELECT_ORDER_TYPE<sub_expr_type>::Result;

    record_range result;
    QUERY_VALUES<sub_expr_type, TOP, ORDER>::select(result, *this, expr);
    return result;
}

} // make
} // db
} // sdl

#endif // __SDL_SYSTEM_MAKETABLE_SELECT_HPP__
