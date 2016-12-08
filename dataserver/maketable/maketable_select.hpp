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

namespace sdl { namespace db { namespace make { namespace make_query_ {

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
    static constexpr operator_ OP = op;
    static constexpr condition cond = T::cond;
};

//--------------------------------------------------------------

template<class T, bool enabled = where_::has_index_hint<T::cond>::value>
struct index_hint;

template<class T>
struct index_hint<T, true> { // allow INDEX::IGNORE|AUTO for any col
    static_assert((T::hint != where_::INDEX::USE) || (T::col::PK || T::col::spatial_key), "INDEX::USE need primary key or spatial index");
    static_assert((T::hint != where_::INDEX::USE) || (T::col::key_pos == 0), "INDEX::USE need key_pos 0");
    static_assert((T::hint != where_::INDEX::USE) || (T::cond != condition::NOT), "INDEX::USE cannot be used with condition::NOT");
    static constexpr where_::INDEX hint = T::hint;
};

template<class T>
struct index_hint<T, false> {
    static constexpr where_::INDEX hint = where_::INDEX::AUTO;
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

template <class T, operator_ OP, operator_ key_op> // T = where_::SEARCH
struct select_is_null {
private:
    enum { temp = where_::is_condition_is_null<T::cond>::value };
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
    template <class T, operator_ OP> using _is_null_OR = select_is_null<T, OP, operator_::OR>;
    template <class T, operator_ OP> using _is_null_AND = select_is_null<T, OP, operator_::AND>;
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

    using is_null_OR = typename search_key<_is_null_OR,
        typename sub_expr_type::type_list,
        typename sub_expr_type::oper_list,
        0
    >::Result;

    using is_null_AND = typename search_key<_is_null_AND,
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
struct RECORD_SELECT : is_static {

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
    template<class record, class expr_type> static bool select(record const & p, expr_type const * const expr, condition_t<condition::STLength>);
    template<class record, class expr_type> static bool select(record const & p, expr_type const * const expr, condition_t<condition::ALL>);
    template<class record, class expr_type> static bool select(record const & p, expr_type const * const expr, condition_t<condition::IS_NULL>);
public:
    template<class record, class sub_expr_type> static
    bool select(record const & p, sub_expr_type const & expr) {
        return select(p, expr.get(Size2Type<T::offset>()), condition_t<T::type::cond>{});
    }
};

template<class T>
template<class record, class expr_type> inline
bool RECORD_SELECT<T>::select(record const & p, expr_type const * const expr, condition_t<condition::ALL>) { 
    return true;
}

template<class T>
template<class record, class expr_type> inline
bool RECORD_SELECT<T>::select(record const & p, expr_type const * const expr, condition_t<condition::IS_NULL>) {
    return p.is_null(identity<typename T::col>{}) == T::type::value;
}

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
    SDL_ASSERT(!(expr->value.values.second < expr->value.values.first));
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
    A_STATIC_CHECK_TYPE(Meters, expr->value.values.second);
    return DISTANCE::compare(
        p.val(identity<typename T::col>{}).STDistance(expr->value.values.first), 
        expr->value.values.second, where_::compare_t<T::type::comp>());
}

template<class T>
template<class record, class expr_type> inline
bool RECORD_SELECT<T>::select(record const & p, expr_type const * const expr, condition_t<condition::STLength>) {
    static_assert(T::col::type == scalartype::t_geography, "STLength need t_geography");
    A_STATIC_CHECK_TYPE(Meters, expr->value.values);
    return DISTANCE::compare(
        p.val(identity<typename T::col>{}).STLength(), 
        expr->value.values, where_::compare_t<T::type::comp>());
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
    static size_t value(sub_expr_type const &, identity<NullType>) {
        return 0;
    }
    template<class T>
    static size_t value(sub_expr_type const & expr, identity<T>) {
        static_assert(TL::Length<T>::value == 1, "TOP duplicate");
        A_STATIC_ASSERT_TYPE(size_t, where_::TOP::value_type);
        return expr.get(Size2Type<T::Head::offset>())->value;
    }
    using TOP_1 = search_condition<
        where_::is_condition_top,
        typename sub_expr_type::type_list,
        typename sub_expr_type::oper_list,
        0>;
public:
    using Result = typename TOP_1::search_where;
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

using algo::stable_sort;

template<class SEARCH_ORDER_BY, stable_sort stable, scalartype::type col_type>
struct record_sort;

template<class SEARCH_ORDER_BY, stable_sort stable, scalartype::type col_type>
struct record_sort {
    using col = typename SEARCH_ORDER_BY::col;
    static constexpr sortorder order = SEARCH_ORDER_BY::type::order;
    template<class record_range, class query_type, class sub_expr_type>
    static void sort(record_range & range, query_type &, sub_expr_type const &) {
        static_assert(col_type == col::type, "");
        static_assert(col_type != scalartype::t_geography, "");
        static_assert(order != sortorder::NONE, "");
        using record = typename record_range::value_type;
        SDL_TRACE_DEBUG_2("record_sort = ", scalartype::get_name(col::type));        
        algo::sort_t<stable>::sort(range, [](record const & x, record const & y){
            return meta::col_less<col, order>::less(
                x.val(identity<col>{}), 
                y.val(identity<col>{}));
        });
    }
};

//FIXME: https://en.wikipedia.org/wiki/Bucket_sort (for unique integer keys)
//FIXME: https://en.wikipedia.org/wiki/Radix_sort (for string keys)
//https://www.cs.princeton.edu/~rs/AlgsDS07/18RadixSort.pdf 

struct record_sort_impl : is_static {

    template<class record, class table_type>
    static void set_table(record & dest, table_type const * table) {
        dest.set_table(table);
    }
    template<class record>
    static auto get_table(const record & p) -> decltype(p.get_table()) {
        return p.get_table();
    }

    template<class table_type>
    union union_type {
        table_type const * table;
        double distance;
        static double get_distance(table_type const * p) {
            static_assert(sizeof(double) <= sizeof(table_type const *), "");
            union_type t;
            t.table = p;
            SDL_ASSERT(t.distance >= 0);
            return t.distance;
        }
    };
};

template<class SEARCH_ORDER_BY, stable_sort stable>
struct record_sort<SEARCH_ORDER_BY, stable, scalartype::t_geography> {
private:
    template<class record_range, class query_type, class sub_expr_type>
    static void sort_impl(record_range &, query_type &, sub_expr_type const &, Int2Type<1>);
    template<class record_range, class query_type, class sub_expr_type>
    static void sort_impl(record_range &, query_type &, sub_expr_type const &, Int2Type<0>);
public:
    using col = typename SEARCH_ORDER_BY::col;
    static constexpr sortorder order = SEARCH_ORDER_BY::type::order;
    template<class record_range, class query_type, class sub_expr_type>
    static void sort(record_range & range, query_type & query, sub_expr_type const & expr) {
        using record = typename record_range::value_type;
        if (!range.empty()) {
            sort_impl(range, query, expr, Int2Type<record::table_type::col_fixed>());
        }
    }
};

template<class SEARCH_ORDER_BY, stable_sort stable>
template<class record_range, class query_type, class sub_expr_type>
void record_sort<SEARCH_ORDER_BY, stable, scalartype::t_geography>::sort_impl(
    record_range & range, query_type & query, sub_expr_type const & expr, Int2Type<1>)
{
    static_assert(scalartype::t_geography == col::type, "");
    static_assert(order != sortorder::NONE, "");
    using record = typename record_range::value_type;
    static_assert(record::table_type::col_fixed, "");
    SDL_TRACE_DEBUG_2("record_sort t_geography = ", scalartype::get_name(col::type));
    SDL_TRACE_DEBUG_2(typeid(SEARCH_ORDER_BY).name());
    SDL_TRACE_DEBUG_2(typeid(col).name());
    SDL_TRACE_DEBUG_2("SEARCH_ORDER_BY::offset=",SEARCH_ORDER_BY::offset);
    const auto & val = expr.get(Size2Type<SEARCH_ORDER_BY::offset>())->value.values;
    A_STATIC_CHECK_TYPE(spatial_point const &, val);
    SDL_ASSERT(val.is_valid());
    SDL_ASSERT(!range.empty());
#if 0 
    using Meters_record = first_second<uint32, record>;// cast Meters to uint32 can improve sort performance (~1 meter accuracy)
    static_assert(sizeof(Meters_record) + 4 == sizeof(first_second<Meters, record>), "");
#else
    using Meters_record = first_second<Meters::value_type, record>;
#endif
    std::vector<Meters_record> temp(range.size());
    {
        auto it = temp.begin();
        for (auto const & x : range) {
            assign_static_cast(it->first, x.val(identity<col>{}).STDistance(val).value());
            it->second = x;
            ++it;
        }
    }
    SDL_ASSERT(temp.size() == range.size());
    algo::sort_t<stable>::sort(temp, [val](Meters_record const & x, Meters_record const & y){
        return value_less<order>::less(x.first, y.first);
    });
    {
        auto it = range.begin();
        for (auto & x : temp) {
            A_STATIC_CHECK_TYPE(record, x.second);
            *it = x.second;
            ++it;
        }
    }
}

template<class SEARCH_ORDER_BY, stable_sort stable>
template<class record_range, class query_type, class sub_expr_type>
void record_sort<SEARCH_ORDER_BY, stable, scalartype::t_geography>::sort_impl(
    record_range & range, query_type & query, sub_expr_type const & expr, Int2Type<0>)
{
    static_assert(scalartype::t_geography == col::type, "");
    static_assert(order != sortorder::NONE, "");
    using record = typename record_range::value_type;
    static_assert(!record::table_type::col_fixed, "");
    SDL_TRACE_DEBUG_2("record_sort t_geography = ", scalartype::get_name(col::type));
    SDL_TRACE_DEBUG_2(typeid(SEARCH_ORDER_BY).name());
    SDL_TRACE_DEBUG_2(typeid(col).name());
    SDL_TRACE_DEBUG_2("SEARCH_ORDER_BY::offset=",SEARCH_ORDER_BY::offset);
    const auto & val = expr.get(Size2Type<SEARCH_ORDER_BY::offset>())->value.values;
    A_STATIC_CHECK_TYPE(spatial_point const &, val);
    SDL_ASSERT(val.is_valid());
    SDL_ASSERT(!range.empty());
    using table_type = typename record::table_type;
    using temp_type = record_sort_impl::union_type<table_type>;
    static_assert(sizeof(temp_type) == sizeof(table_type const *), "");
    table_type const * const restore = record_sort_impl::get_table(range[0]);
    SDL_ASSERT(restore);
    temp_type temp = {};
    for (auto & x : range) {
        temp.distance = x.val(identity<col>{}).STDistance(val).value();
        SDL_ASSERT(record_sort_impl::get_table(x) == restore); // all records must refer to the same table
        record_sort_impl::set_table(x, temp.table);
    }
    algo::sort_t<stable>::sort(range, [val](record const & x, record const & y) { // sort range by STDistance in-place
        return value_less<order>::less(
            temp_type::get_distance(record_sort_impl::get_table(x)),
            temp_type::get_distance(record_sort_impl::get_table(y)));
    });
    for (auto & x : range) {
        record_sort_impl::set_table(x, restore);
    }
}

//--------------------------------------------------------------

template<class TList> struct SORT_RECORD_RANGE;
template<> struct SORT_RECORD_RANGE<NullType>
{
    template<typename... Ts>
    static void sort(Ts&&...){}
};

template<class Head>
struct SORT_RECORD_RANGE<Typelist<Head, NullType>>
{
    template<class record_range, class query_type, class sub_expr_type>
    static void sort(record_range & range, query_type & query, sub_expr_type const & expr) {
        A_STATIC_CHECK_TYPE(const scalartype::type, Head::col::type);
        record_sort<Head, stable_sort::false_, Head::col::type>::sort(range, query, expr);
    }
};

template<class Head, class Tail>
struct SORT_RECORD_RANGE<Typelist<Head, Tail>>  // Head = SEARCH_WHERE<where_::ORDER_BY>
{
    template<class record_range, class query_type, class sub_expr_type>
    static void sort(record_range & range, query_type & query, sub_expr_type const & expr) {
        A_STATIC_ASSERT_NOT_TYPE(Tail, NullType);
        A_STATIC_CHECK_TYPE(const scalartype::type, Head::col::type);
        SORT_RECORD_RANGE<Tail>::sort(range, query, expr);
        record_sort<Head, stable_sort::true_, Head::col::type>::sort(range, query, expr);
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
    enum { is_null_OR = TL::Length<typename KEYS::is_null_OR>::value };
    enum { lambda_OR_null = lambda_OR || is_null_OR };
public:
    enum { use_index = !lambda_OR_null && (key_AND_0 || (key_OR_0 && !no_key_OR_0)) };
    enum { spatial_index = !use_index && !lambda_OR_null && (spatial_AND || (spatial_OR && !no_key_OR_0)) };

#if SDL_DEBUG_QUERY
    static void trace(){
        SDL_TRACE("key_OR_0 = ", key_OR_0);
        SDL_TRACE("key_AND_0 = ", key_AND_0);
        SDL_TRACE("no_key_OR_0 = ", no_key_OR_0);
        SDL_TRACE("no_key_AND_0 = ", no_key_AND_0);
        SDL_TRACE("spatial_OR = ", spatial_OR);
        SDL_TRACE("spatial_AND = ", spatial_AND);
        SDL_TRACE("lambda_OR = ", lambda_OR);
        SDL_TRACE("is_null_OR = ", is_null_OR);
        SDL_TRACE("use_index = ", use_index);
        SDL_TRACE("spatial_index = ", spatial_index);
    }
#endif
};

template<class sub_expr_type>
struct IS_SCAN_TABLE {
    using seek_sub_expr = IS_SEEK_TABLE<sub_expr_type>;
public:
    enum { value = !seek_sub_expr::spatial_index && !seek_sub_expr::use_index };
};

//--------------------------------------------------------------

template <class TList> struct order_cluster;
template <> struct order_cluster<NullType>
{
    using Result = NullType;
};

template <class T, class Tail>
struct order_cluster< Typelist<T, Tail>> { // T = SEARCH_WHERE<where_::ORDER_BY>
private:
    enum { value = T::col::PK && (0 == T::col::key_pos) && (T::type::order == T::col::order) };
public:
    using Result = Select_t<value, T, typename order_cluster<Tail>::Result>;
};

//--------------------------------------------------------------

template<class sub_expr_type>
struct SELECT_ORDER_TYPE {
private:
    using ORDER_1 = search_condition<
        where_::is_condition_order,
        typename sub_expr_type::type_list,
        typename sub_expr_type::oper_list,
        0>;
    using ORDER_2 = typename ORDER_1::search_where;
    static_assert(CHECK_ORDER<ORDER_2>::value, "SELECT_ORDER_TYPE");
    using cluster_type = typename order_cluster<ORDER_2>::Result;
    enum { scan_table = IS_SCAN_TABLE<sub_expr_type>::value };
    enum { cluster_order = (TL::IndexOf<ORDER_2, cluster_type>::value == 0) && (TL::Length<ORDER_2>::value == 1) };
    enum { remove_order = scan_table && cluster_order };
    using ORDER_3 = Select_t<remove_order, NullType, ORDER_2>;
public:
    using Result = ORDER_3;
};

//--------------------------------------------------------------

template<class record_range, class query_type, class sub_expr_type, bool is_limit>
class SCAN_TABLE final : noncopyable {

    using record = typename query_type::record;
    using SEARCH = typename SELECT_SEARCH_TYPE<sub_expr_type>::Result;
    using search_AND = search_operator_t<operator_::AND, SEARCH>;
    using search_OR = search_operator_t<operator_::OR, SEARCH>;
    static_assert(TL::Length<search_OR>::value, "empty OR");

    static bool has_limit(bool, std::false_type) {
        return false;
    }
    bool has_limit(bool check, std::true_type) const {
        return check && (m_limit <= m_result.size());
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
        static_assert(IS_SCAN_TABLE<sub_expr_type>::value, "SCAN_TABLE");
    }
    void select();
};

template<class record_range, class query_type, class sub_expr_type, bool is_limit>
void SCAN_TABLE<record_range, query_type, sub_expr_type, is_limit>::select() {
    m_query.scan_if([this](record const p){
        if (is_select(p)) {
            auto const push_result = query_type::push_back(m_result, p);
            if (push_result.first == bc::break_) {
                return false;
            }
            if (has_limit(push_result.second, bool_constant<is_limit>{}))
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

    static_assert(query_type::index_size, "seek_table need index_size");

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

    template<class expr_type, class fun_type> static break_or_continue scan_greater(query_type &, expr_type const *, fun_type &&);
    template<class expr_type, class fun_type> static break_or_continue scan_greater_eq(query_type &, expr_type const *, fun_type &&);

    template<class expr_type, class fun_type> static break_or_continue scan_less_with_sortorder(query_type &, expr_type const *, fun_type &&, sortorder_t<sortorder::ASC>);
    template<class expr_type, class fun_type> static break_or_continue scan_less_with_sortorder(query_type &, expr_type const *, fun_type &&, sortorder_t<sortorder::DESC>);

    template<class expr_type, class fun_type> static break_or_continue scan_less_eq_with_sortorder(query_type &, expr_type const *, fun_type &&, sortorder_t<sortorder::ASC>);
    template<class expr_type, class fun_type> static break_or_continue scan_less_eq_with_sortorder(query_type &, expr_type const *, fun_type &&, sortorder_t<sortorder::DESC>);

    template<class expr_type, class fun_type> static break_or_continue scan_greater_with_sortorder(query_type &, expr_type const *, fun_type &&, sortorder_t<sortorder::ASC>);
    template<class expr_type, class fun_type> static break_or_continue scan_greater_with_sortorder(query_type &, expr_type const *, fun_type &&, sortorder_t<sortorder::DESC>);

    template<class expr_type, class fun_type> static break_or_continue scan_greater_eq_with_sortorder(query_type &, expr_type const *, fun_type &&, sortorder_t<sortorder::ASC>);
    template<class expr_type, class fun_type> static break_or_continue scan_greater_eq_with_sortorder(query_type &, expr_type const *, fun_type &&, sortorder_t<sortorder::DESC>);

    template<class expr_type, class fun_type> static break_or_continue scan_between(query_type &, expr_type const *, fun_type &&, sortorder_t<sortorder::ASC>);
    template<class expr_type, class fun_type> static break_or_continue scan_between(query_type &, expr_type const *, fun_type &&, sortorder_t<sortorder::DESC>);

    // T = make_query_::SEARCH_WHERE
    template<class expr_type, class fun_type, class T> static break_or_continue scan_if(query_type &, expr_type const *, fun_type &&, identity<T>, condition_t<condition::WHERE>);
    template<class expr_type, class fun_type, class T> static break_or_continue scan_if(query_type &, expr_type const *, fun_type &&, identity<T>, condition_t<condition::IN>);
    template<class expr_type, class fun_type, class T> static break_or_continue scan_if(query_type &, expr_type const *, fun_type &&, identity<T>, condition_t<condition::LESS>);
    template<class expr_type, class fun_type, class T> static break_or_continue scan_if(query_type &, expr_type const *, fun_type &&, identity<T>, condition_t<condition::GREATER>);
    template<class expr_type, class fun_type, class T> static break_or_continue scan_if(query_type &, expr_type const *, fun_type &&, identity<T>, condition_t<condition::LESS_EQ>);
    template<class expr_type, class fun_type, class T> static break_or_continue scan_if(query_type &, expr_type const *, fun_type &&, identity<T>, condition_t<condition::GREATER_EQ>);
    template<class expr_type, class fun_type, class T> static break_or_continue scan_if(query_type &, expr_type const *, fun_type &&, identity<T>, condition_t<condition::BETWEEN>);
public:
    template<class expr_type, class fun_type, class T> 
    static break_or_continue scan_if(query_type & query, expr_type const * v, fun_type && fun, identity<T>) {
        static_assert(T::cond != condition::NOT, "");
        static_assert(T::cond < condition::lambda, "");
        return seek_table::scan_if(query, v, fun, identity<T>{}, condition_t<T::cond>{});
    }
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
template<class expr_type, class fun_type> break_or_continue
make_query<this_table, _record>::seek_table::scan_greater(query_type & query, expr_type const * const expr, fun_type && fun)
{
    break_or_continue result = bc::continue_;
    auto found = query.lower_bound(expr->value.values);
    if (found.first.page) {
        if (found.second) { // skip equal values
            found.first = query.scan_next(found.first, [expr](record const & p){
                return is_equal::apply(p, expr->value.values);
            });
        }
        if (found.first.page) {
            SDL_ASSERT(is_less<invert_sortorder<col_type::order>::value>::apply
                (query.get_record(found.first), expr->value.values));
            query.scan_next(found.first, [&result, &fun](record const & p){
                return bc::continue_ == (result = fun(p));
            });
        }
    }
    return result;
}

template<class this_table, class _record> 
template<class expr_type, class fun_type> break_or_continue
make_query<this_table, _record>::seek_table::scan_greater_eq(query_type & query, expr_type const * const expr, fun_type && fun)
{
    break_or_continue result = bc::continue_;
    auto found = query.lower_bound(expr->value.values);
    if (found.first.page) {
        SDL_ASSERT(is_less_eq<invert_sortorder<col_type::order>::value>::apply(query.get_record(found.first), expr->value.values));
        query.scan_next(found.first, [&result, &fun](record const & p){
            return bc::continue_ == (result = fun(p));
        });
    }
    return result;
}

//--------------------------------------------------------------------------------

template<class this_table, class _record> 
template<class expr_type, class fun_type> inline break_or_continue
make_query<this_table, _record>::seek_table::scan_less_with_sortorder(query_type & query, expr_type const * const expr, fun_type && fun, sortorder_t<sortorder::ASC>) {
    static_assert(col_type::order == sortorder::ASC, "");
    return scan_less(query, expr, fun, identity<is_less<sortorder::ASC>>{});
}

template<class this_table, class _record> 
template<class expr_type, class fun_type> inline break_or_continue
make_query<this_table, _record>::seek_table::scan_less_with_sortorder(query_type & query, expr_type const * const expr, fun_type && fun, sortorder_t<sortorder::DESC>) {
    static_assert(col_type::order == sortorder::DESC, "");
    return scan_greater(query, expr, fun);
}

//--------------------------------------------------------------------------------

template<class this_table, class _record> 
template<class expr_type, class fun_type> inline break_or_continue
make_query<this_table, _record>::seek_table::scan_less_eq_with_sortorder(query_type & query, expr_type const * const expr, fun_type && fun, sortorder_t<sortorder::ASC>) {
    static_assert(col_type::order == sortorder::ASC, "");
    return scan_less(query, expr, fun, identity<is_less_eq<sortorder::ASC>>{});
}

template<class this_table, class _record> 
template<class expr_type, class fun_type> inline break_or_continue
make_query<this_table, _record>::seek_table::scan_less_eq_with_sortorder(query_type & query, expr_type const * const expr, fun_type && fun, sortorder_t<sortorder::DESC>) {
    static_assert(col_type::order == sortorder::DESC, "");
    return scan_greater_eq(query, expr, fun);
}

//--------------------------------------------------------------------------------

template<class this_table, class _record> 
template<class expr_type, class fun_type> inline break_or_continue
make_query<this_table, _record>::seek_table::scan_greater_with_sortorder(query_type & query, expr_type const * const expr, fun_type && fun, sortorder_t<sortorder::ASC>) {
    static_assert(col_type::order == sortorder::ASC, "");
    return scan_greater(query, expr, fun);
}

template<class this_table, class _record> 
template<class expr_type, class fun_type> inline break_or_continue
make_query<this_table, _record>::seek_table::scan_greater_with_sortorder(query_type & query, expr_type const * const expr, fun_type && fun, sortorder_t<sortorder::DESC>) {
    static_assert(col_type::order == sortorder::DESC, "");
    return scan_less(query, expr, fun, identity<is_less<sortorder::DESC>>{});
}

//--------------------------------------------------------------------------------

template<class this_table, class _record> 
template<class expr_type, class fun_type> inline break_or_continue
make_query<this_table, _record>::seek_table::scan_greater_eq_with_sortorder(query_type & query, expr_type const * const expr, fun_type && fun, sortorder_t<sortorder::ASC>) {
    static_assert(col_type::order == sortorder::ASC, "");
    return scan_greater_eq(query, expr, fun);
}

template<class this_table, class _record> 
template<class expr_type, class fun_type> inline break_or_continue
make_query<this_table, _record>::seek_table::scan_greater_eq_with_sortorder(query_type & query, expr_type const * const expr, fun_type && fun, sortorder_t<sortorder::DESC>) {
    static_assert(col_type::order == sortorder::DESC, "");
    return scan_less(query, expr, fun, identity<is_less_eq<sortorder::DESC>>{});
}

//--------------------------------------------------------------------------------

template<class this_table, class _record> 
template<class expr_type, class fun_type> inline break_or_continue
make_query<this_table, _record>::seek_table::scan_between(query_type & query, expr_type const * const expr, fun_type && fun, sortorder_t<sortorder::ASC>) {
    static_assert(col_type::order == sortorder::ASC, "");
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

template<class this_table, class _record> 
template<class expr_type, class fun_type> break_or_continue
make_query<this_table, _record>::seek_table::scan_between(query_type & query, expr_type const * const expr, fun_type && fun, sortorder_t<sortorder::DESC>) {
    static_assert(col_type::order == sortorder::DESC, "");
    break_or_continue result = bc::continue_;
    auto found = query.lower_bound(expr->value.values.second);
    if (found.first.page) {
        query.scan_next(found.first, [&result, expr, &fun](record const & p){
            if (is_less_eq<sortorder::DESC>::apply(p, expr->value.values.first)) {
                return bc::continue_ == (result = fun(p));
            }
            return false;
        });
    }
    return result;
}

//--------------------------------------------------------------------------------

template<class this_table, class _record> 
template<class expr_type, class fun_type, class T> inline break_or_continue
make_query<this_table, _record>::seek_table::scan_if(query_type & query, expr_type const * const expr, fun_type && fun, identity<T>, condition_t<condition::LESS>) {
    return scan_less_with_sortorder(query, expr, fun, sortorder_t<col_type::order>{});
}

template<class this_table, class _record> 
template<class expr_type, class fun_type, class T> inline break_or_continue
make_query<this_table, _record>::seek_table::scan_if(query_type & query, expr_type const * const expr, fun_type && fun, identity<T>, condition_t<condition::LESS_EQ>) {
    return scan_less_eq_with_sortorder(query, expr, fun, sortorder_t<col_type::order>{});
}

//--------------------------------------------------------------------------------

template<class this_table, class _record> 
template<class expr_type, class fun_type, class T> inline break_or_continue
make_query<this_table, _record>::seek_table::scan_if(query_type & query, expr_type const * const expr, fun_type && fun, identity<T>, condition_t<condition::GREATER>) {
    return scan_greater_with_sortorder(query, expr, fun, sortorder_t<col_type::order>{});
}

template<class this_table, class _record> 
template<class expr_type, class fun_type, class T> inline break_or_continue
make_query<this_table, _record>::seek_table::scan_if(query_type & query, expr_type const * const expr, fun_type && fun, identity<T>, condition_t<condition::GREATER_EQ>) {
    return scan_greater_eq_with_sortorder(query, expr, fun, sortorder_t<col_type::order>{});
}

//--------------------------------------------------------------------------------

template<class this_table, class _record> 
template<class expr_type, class fun_type, class T> inline break_or_continue
make_query<this_table, _record>::seek_table::scan_if(query_type & query, expr_type const * const expr, fun_type && fun, identity<T>, condition_t<condition::BETWEEN>) {
    SDL_ASSERT(!(expr->value.values.second < expr->value.values.first));
    return scan_between(query, expr, fun, sortorder_t<col_type::order>{});
}

//////////////////////////////////////////////////////////////////////////////////////////

template<class this_table, class _record> 
class make_query<this_table, _record>::seek_spatial final : is_static
{
    using query_type = make_query<this_table, _record>;
    using record = typename query_type::record;
    using pk0_type = typename query_type::T0_type;
    using spatial_page_row = typename query_type::spatial_page_row;

    static_assert(query_type::index_size == 1, "seek_spatial"); //composite keys not implemented
    template<class fun_type> static break_or_continue
    find_record(spatial_page_row const * const row, query_type & query, fun_type && fun) {
        if (auto found = query.find_with_index_n(row->data.pk0)) { // found record
            A_STATIC_CHECK_TYPE(record, found);
            return fun(found);
        }
        SDL_ASSERT(!"bad primary key");
        return bc::break_;
    }
    template<class T, class expr_type>
    static bool STIntersects(expr_type const * const expr, record const & p, where_::intersect_hint_t<where_::intersect_hint::precise>) {
        static_assert(T::type::inter == where_::intersect_hint::precise, "");
        return p.val(identity<typename T::col>{}).STIntersects(expr->value.values);
    } 
    template<class T, class expr_type>
    static bool STIntersects(expr_type const *, record const &, where_::intersect_hint_t<where_::intersect_hint::fast>) {
        static_assert(T::type::inter == where_::intersect_hint::fast, "");
        return true;
    }
    template<class T, class expr_type>
    static bool STDistance(expr_type const * const expr, record const & p, where_::intersect_hint_t<where_::intersect_hint::precise>) {
        static_assert(T::type::inter == where_::intersect_hint::precise, "");
        static_assert(T::type::comp < where_::compare::greater, "");
        return make_query_::DISTANCE::compare(
                p.val(identity<typename T::col>{}).STDistance(expr->value.values.first), 
                expr->value.values.second,
                where_::compare_t<T::type::comp>());
    } 
    template<class T, class expr_type>
    static bool STDistance(expr_type const *, record const &, where_::intersect_hint_t<where_::intersect_hint::fast>) {
        static_assert(T::type::inter == where_::intersect_hint::fast, "");
        static_assert(T::type::comp < where_::compare::greater, "");
        return true;
    }
public:
    // T = make_query_::SEARCH_WHERE
    template<class expr_type, class fun_type, class T> static break_or_continue scan_if(query_type &, expr_type const *, fun_type &&, identity<T>, condition_t<condition::STContains>);
    template<class expr_type, class fun_type, class T> static break_or_continue scan_if(query_type &, expr_type const *, fun_type &&, identity<T>, condition_t<condition::STIntersects>);
    template<class expr_type, class fun_type, class T> static break_or_continue scan_if(query_type &, expr_type const *, fun_type &&, identity<T>, condition_t<condition::STDistance>);
};

template<class this_table, class _record>
template<class expr_type, class fun_type, class T> break_or_continue
make_query<this_table, _record>::seek_spatial::scan_if(query_type & query, expr_type const * const expr, fun_type && fun, identity<T>, condition_t<condition::STContains>) {
    A_STATIC_CHECK_TYPE(spatial_point, expr->value.values);
    static_assert(T::col::type == scalartype::t_geography, "STContains need t_geography");
    if (auto tree = query.get_spatial_tree()) {
        sparse_set_t<pk0_type> m_pk0; // check processed records
        return tree->for_point(expr->value.values,
            [&m_pk0, &query, expr, &fun](spatial_page_row const * const row) {
                if (m_pk0.insert(row->data.pk0)) {
                    if (row->cell_cover()) {
                        return seek_spatial::find_record(row, query, fun);
                    }
                    return seek_spatial::find_record(row, query, [expr, &fun](record const & p){
                        if (p.val(identity<typename T::col>{}).STContains(expr->value.values)) {
                           return fun(p);
                        }
                        return bc::continue_;
                    });
                }
                return bc::continue_;
            });
    }
    SDL_ASSERT(0);
    return bc::break_;
}

template<class this_table, class _record>
template<class expr_type, class fun_type, class T> break_or_continue
make_query<this_table, _record>::seek_spatial::scan_if(query_type & query, expr_type const * const expr, fun_type && fun, identity<T>, condition_t<condition::STIntersects>) {
    A_STATIC_CHECK_TYPE(spatial_rect, expr->value.values);
    static_assert(T::col::type == scalartype::t_geography, "STIntersects need t_geography");
    if (auto tree = query.get_spatial_tree()) {
        sparse_set_t<pk0_type> m_pk0; // check processed records
        return tree->for_rect(expr->value.values, 
            [&m_pk0, &query, expr, &fun](spatial_page_row const * const row) {
                if (m_pk0.insert(row->data.pk0)) {
                    if (row->cell_cover()) {
                        return seek_spatial::find_record(row, query, fun);
                    }
                    return seek_spatial::find_record(row, query, [expr, &fun](record const & p){
                        if (seek_spatial::STIntersects<T>(expr, p, where_::intersect_hint_t<T::type::inter>())) {
                            return fun(p);
                        }
                        return bc::continue_;
                    });
                }
                return bc::continue_;
            });
    }
    SDL_ASSERT(0);
    return bc::break_;
}

template<class this_table, class _record>
template<class expr_type, class fun_type, class T> break_or_continue
make_query<this_table, _record>::seek_spatial::scan_if(query_type & query, expr_type const * const expr, fun_type && fun, identity<T>, condition_t<condition::STDistance>) {
    static_assert(T::col::type == scalartype::t_geography, "STDistance need t_geography");
    A_STATIC_CHECK_TYPE(spatial_point, expr->value.values.first);
    A_STATIC_CHECK_TYPE(Meters, expr->value.values.second);
    if (auto tree = query.get_spatial_tree()) {
        static_assert(where_::for_range<T::type::comp>::value, "STDistance use_for_range");
        sparse_set_t<pk0_type> m_pk0; // check processed records
        return tree->for_range(expr->value.values,
            [&m_pk0, &query, expr, &fun](spatial_page_row const * const row) {
                if (m_pk0.insert(row->data.pk0)) {
                    if (row->cell_cover()) { // STDistance = 0
                        if (make_query_::DISTANCE::compare(Meters(0),
                                expr->value.values.second,
                                where_::compare_t<T::type::comp>()))
                        {
                            return seek_spatial::find_record(row, query, fun);
                        }
                        return bc::continue_;
                    }
                    return seek_spatial::find_record(row, query, [expr, &fun](record const & p){
                        if (seek_spatial::STDistance<T>(expr, p, where_::intersect_hint_t<T::type::inter>())) {
                            return fun(p);
                        }
                        return bc::continue_;
                    });
                }
                return bc::continue_;
            });
    }
    SDL_ASSERT(0);
    return bc::break_;
}


//////////////////////////////////////////////////////////////////////////////////////////

namespace make_query_ {

template<class record_range, class query_type, class sub_expr_type, bool is_limit>
class SEEK_TABLE final : noncopyable {

    using this_type = SEEK_TABLE;
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

template<class record_range, class query_type, class sub_expr_type, bool is_limit> inline
bool SEEK_TABLE<record_range, query_type, sub_expr_type, is_limit>::is_select(record const & p, operator_t<operator_::OR>) const
{
    return SELECT_AND<typename KEYS::search_AND, true>::select(p, m_expr); // must be
}

template<class record_range, class query_type, class sub_expr_type, bool is_limit> inline
bool SEEK_TABLE<record_range, query_type, sub_expr_type, is_limit>::is_select(record const & p, operator_t<operator_::AND>) const
{
    return 
        SELECT_OR<typename KEYS::search_OR, true>::select(p, m_expr) &&     // any of 
        SELECT_AND<typename KEYS::no_key_AND_0, true>::select(p, m_expr);   // must be
}

template<class record_range, class query_type, class sub_expr_type, bool is_limit>
template<class expr_type, class T> inline // T = SEARCH_WHERE
bool SEEK_TABLE<record_range, query_type, sub_expr_type, is_limit>::seek_with_index(expr_type const * const expr, identity<T>)
{
    return query_type::seek_table::scan_if(m_query, expr, [this](record const p) {
        if (is_select(p, operator_t<T::OP>{})) { // check other part of condition 
            A_STATIC_ASSERT_NOT_TYPE(void, typename key_type::this_clustered);
            if (query_type::push_unique(m_result, p) && has_limit(bool_constant<is_limit>{})) {
                return bc::break_;
            }
        }
        return bc::continue_;
    }, 
    identity<T>{}) == bc::continue_;
}

template<class record_range, class query_type, class sub_expr_type, bool is_limit> inline
void SEEK_TABLE<record_range, query_type, sub_expr_type, is_limit>::select()
{
    using keylist = Select_t<TL::IsEmpty<key_AND_0>::value, key_OR_0, key_AND_0>;
    meta::processor_if<keylist>::apply(seek_with_index_t(this));
}

//---------------------------------------------------------------------------------

template<class record_range, class query_type, class sub_expr_type, bool is_limit>
class SEEK_SPATIAL final : noncopyable {

    using this_type = SEEK_SPATIAL;
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

template<class record_range, class query_type, class sub_expr_type, bool is_limit> inline
bool SEEK_SPATIAL<record_range, query_type, sub_expr_type, is_limit>::is_select(record const & p, operator_t<operator_::OR>) const
{
    return SELECT_AND<typename KEYS::search_AND, true>::select(p, m_expr); // must be
}

template<class record_range, class query_type, class sub_expr_type, bool is_limit> inline
bool SEEK_SPATIAL<record_range, query_type, sub_expr_type, is_limit>::is_select(record const & p, operator_t<operator_::AND>) const
{
    return 
        SELECT_OR<typename KEYS::search_OR, true>::select(p, m_expr) &&     // any of 
        SELECT_AND<typename KEYS::no_key_AND_0, true>::select(p, m_expr);   // must be
}

template<class record_range, class query_type, class sub_expr_type, bool is_limit>
template<class expr_type, class T> inline // T = SEARCH_WHERE
bool SEEK_SPATIAL<record_range, query_type, sub_expr_type, is_limit>::seek_with_index(expr_type const * const expr, identity<T>)
{
    return query_type::seek_spatial::scan_if(m_query, expr, [this](record const p) {
        if (is_select(p, operator_t<T::OP>{})) { // check other part of condition 
            A_STATIC_ASSERT_NOT_TYPE(void, typename key_type::this_clustered);
            auto const push_result = make_break_or_continue_bool(query_type::push_unique(m_result, p));
            if (push_result.first == bc::break_) {
                return bc::break_;
            }
            if (push_result.second && has_limit(bool_constant<is_limit>{})) {
                return bc::break_;
            }
        }
        return bc::continue_;
    }, 
    identity<T>{}, condition_t<T::cond>{}) == bc::continue_;
}

template<class record_range, class query_type, class sub_expr_type, bool is_limit> inline
void SEEK_SPATIAL<record_range, query_type, sub_expr_type, is_limit>::select()
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
    using scan_table_type = SCAN_TABLE<record_range, query_type, sub_expr_type, is_limit>;
    using seek_table_type = SEEK_TABLE<record_range, query_type, sub_expr_type, is_limit>;
    using seek_spatial_type = SEEK_SPATIAL<record_range, query_type, sub_expr_type, is_limit>;
    using seek_sub_expr = IS_SEEK_TABLE<sub_expr_type>;
#if SDL_DEBUG_QUERY
    seek_sub_expr::trace();
#endif
    using select_table =
        Select_t<seek_sub_expr::spatial_index, seek_spatial_type, 
        Select_t<seek_sub_expr::use_index, seek_table_type, scan_table_type>>;
    select_table(result, query, expr, limit(expr)).select();
}

//--------------------------------------------------------------

template<class sub_expr_type, class TOP, class ORDER>
struct QUERY_VALUES
{
    static_assert(TL::Length<TOP>::value == 1, "TOP");
    static_assert(TL::Length<ORDER>::value, "ORDER");

    template<class record_range, class query_type> static
    void select(record_range & result, query_type & query, sub_expr_type const & expr) {
        //FIXME: can be optimized for some cases
        SCAN_OR_SEEK<sub_expr_type>::select(result, query, expr);
        SORT_RECORD_RANGE<ORDER>::sort(result, query, expr);
        result.resize(a_min(SELECT_TOP(expr), result.size()));
        result.shrink_to_fit();
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
        SORT_RECORD_RANGE<ORDER>::sort(result, query, expr);
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
    SDL_TRACE_QUERY("\n------");
#endif
    using TOP = typename SELECT_TOP_TYPE<sub_expr_type>::Result;
    using ORDER = typename SELECT_ORDER_TYPE<sub_expr_type>::Result;

#if SDL_DEBUG_QUERY
    SDL_TRACE("IS_SCAN_TABLE = ", IS_SCAN_TABLE<sub_expr_type>::value);
    SDL_TRACE("ORDER = ", TL::Length<ORDER>::value);
#endif
    record_range result;
    QUERY_VALUES<sub_expr_type, TOP, ORDER>::select(result, *this, expr);
    return result;
}

template<class this_table, class record>
template<class sub_expr_type, class fun_type>
void make_query<this_table, record>::for_record(sub_expr_type const & expr, fun_type && result)
{
    using namespace make_query_;
    static_assert(CHECK_INDEX<sub_expr_type>::value, "");

#if SDL_DEBUG_QUERY
    SDL_TRACE_QUERY("\nVALUES:");
    where_::trace_::trace_sub_expr(expr);
    SDL_TRACE_QUERY("\n------");
#endif
    using TOP = typename SELECT_TOP_TYPE<sub_expr_type>::Result;
    using ORDER = typename SELECT_ORDER_TYPE<sub_expr_type>::Result;

    static_assert(TL::IsEmpty<TOP>::value, "for_record TOP");
    static_assert(TL::IsEmpty<ORDER>::value, "for_record ORDER");

    QUERY_VALUES<sub_expr_type, TOP, ORDER>::select(result, *this, expr);
}

} // make
} // db
} // sdl

#endif // __SDL_SYSTEM_MAKETABLE_SELECT_HPP__
