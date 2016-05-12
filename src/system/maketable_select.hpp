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
struct index_hint<T, true> {
    static_assert((T::hint != where_::INDEX::USE) || T::col::PK, "INDEX::USE need primary key");
    static_assert((T::hint != where_::INDEX::USE) || (T::col::key_pos == 0), "INDEX::USE need key_pos 0");
    static_assert((T::hint != where_::INDEX::USE) || (T::cond != condition::NOT), "INDEX::USE cannot be with condition::NOT");
    static const where_::INDEX hint = T::hint;
};

template<class T>
struct index_hint<T, false> {
    static const where_::INDEX hint = where_::INDEX::AUTO;
};

//--------------------------------------------------------------

template<class T, size_t key_pos, bool enabled = where_::is_condition_index<T::cond>::value>
struct use_index;

template<class T, size_t key_pos>
struct use_index<T, key_pos, true> {
private:
    static_assert(T::hint == index_hint<T>::hint, "use_index");
public:
    enum { value = T::col::PK && (T::col::key_pos == key_pos) && (T::hint != where_::INDEX::IGNORE) };
};

template<class T, size_t key_pos>
struct use_index<T, key_pos, false> {
private:
    static_assert(index_hint<T>::hint == where_::INDEX::AUTO, "use_index");
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
    enum { search = where_::is_condition_search<T::cond>::value };
    enum { temp = search && !use_index<T, key_pos>::value };
public:
    enum { value = temp && (OP == key_op) };
};

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
    template <class T, operator_ OP> using _key_AND_0 = select_key<T, OP, 0, operator_::AND>; //FIXME: exclude condition::NOT
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
    enum { lambda_OR = TL::Length<typename KEYS::lambda_OR>::value };
public:
    enum { value = !lambda_OR && (key_AND_0 || (key_OR_0 && !no_key_OR_0)) };

#if SDL_DEBUG_QUERY
    static void trace(){
        SDL_TRACE("key_OR_0 = ", key_OR_0);
        SDL_TRACE("key_AND_0 = ", key_AND_0);
        SDL_TRACE("no_key_OR_0 = ", no_key_OR_0);
        SDL_TRACE("no_key_AND_0 = ", no_key_AND_0);
        SDL_TRACE("lambda_OR = ", lambda_OR);
        SDL_TRACE("IS_SEEK_TABLE = ", value);
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
        static_assert(!IS_SEEK_TABLE<sub_expr_type>::value, "SCAN_TABLE");
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
class make_query<this_table, _record>::seek_table : is_static
{
    using query_type = make_query<this_table, _record>;
    using record = typename query_type::record;

    static_assert(query_type::index_size, "");
    static_assert(query_type::T0_col::order == sortorder::ASC, "seek_table need sortorder::ASC"); //FIXME: sortorder::DESC...

    enum { is_composite = query_type::index_size > 1 };

    template<class value_type, class fun_type> static break_or_continue scan_where(query_type &, value_type const &, fun_type, std::false_type);
    template<class value_type, class fun_type> static break_or_continue scan_where(query_type &, value_type const &, fun_type, std::true_type);
    template<class value_type, class fun_type> static break_or_continue scan_where(query_type & query, value_type const & value, fun_type fun) {    
        return scan_where(query, value, fun, bool_constant<is_composite>{});
    }
    template<class col, sortorder ord>// = sortorder::ASC>
    struct is_between {
        using val_type = typename col::val_type;
        val_type const & second;
        explicit is_between(val_type const & v): second(v){}
        bool operator()(val_type const & x, val_type const &) const {
            return meta::col_less<col, ord>::less(x, second)
                || meta::is_equal<col>::equal(x, second);
        }
    };
    template<class col>
    struct is_equal {
        template<class record, class value_type> static
        bool apply(record const & p, value_type const & v) {
            return meta::is_equal<col>::equal(p.val(identity<col>{}),
                static_cast<typename col::val_type const &>(v));
        }
        template<class record, class value_type>
        bool operator()(record const & p, value_type const & v) const {
            return apply(p, v);
        }
    };
    template<class col, sortorder ord>
    struct is_less {
        template<class record, class value_type> static
        bool apply(record const & p, value_type const & v) {
            return meta::col_less<col, ord>::less(p.val(identity<col>{}), 
                static_cast<typename col::val_type const &>(v));
        }
        template<class record, class value_type>
        bool operator()(record const & p, value_type const & v) const {
            return apply(p, v);
        }
    };
    template<class col, sortorder ord>
    struct is_less_eq {
        template<class record, class value_type> static
        bool apply(record const & p, value_type const & v) {
            return is_less<col, ord>::apply(p, v) || is_equal<col>::apply(p, v);
        }
        template<class record, class value_type>
        bool operator()(record const & p, value_type const & v) const {
            return apply(p, v);
        }
    };
    template<class expr_type, class fun_type, class less_type> static
    break_or_continue scan_less(query_type &, expr_type const *, fun_type, identity<less_type>);
public:
    // T = make_query_::SEARCH_WHERE
    template<class expr_type, class fun_type, class T> static break_or_continue scan_if(query_type &, expr_type const *, fun_type, identity<T>, condition_t<condition::WHERE>);
    template<class expr_type, class fun_type, class T> static break_or_continue scan_if(query_type &, expr_type const *, fun_type, identity<T>, condition_t<condition::IN>);
    template<class expr_type, class fun_type, class T> static break_or_continue scan_if(query_type &, expr_type const *, fun_type, identity<T>, condition_t<condition::LESS>);
    template<class expr_type, class fun_type, class T> static break_or_continue scan_if(query_type &, expr_type const *, fun_type, identity<T>, condition_t<condition::GREATER>);
    template<class expr_type, class fun_type, class T> static break_or_continue scan_if(query_type &, expr_type const *, fun_type, identity<T>, condition_t<condition::LESS_EQ>);
    template<class expr_type, class fun_type, class T> static break_or_continue scan_if(query_type &, expr_type const *, fun_type, identity<T>, condition_t<condition::GREATER_EQ>);
    template<class expr_type, class fun_type, class T> static break_or_continue scan_if(query_type &, expr_type const *, fun_type, identity<T>, condition_t<condition::BETWEEN>);
private:
    template<class expr_type, class fun_type, class T> static void scan_if(query_type &, expr_type const *, fun_type, identity<T>, condition_t<condition::NOT>) {} // not called
};

template<class this_table, class _record> template<class value_type, class fun_type> inline break_or_continue
make_query<this_table, _record>::seek_table::scan_where(query_type & query, value_type const & v, fun_type fun, std::false_type) {
    static_assert(!is_composite, "");
    if (auto found = query.find_with_index(query_type::make_key(v))) {
        return fun(found);
    }
    return bc::continue_;
}

template<class this_table, class _record> template<class value_type, class fun_type> inline break_or_continue
make_query<this_table, _record>::seek_table::scan_where(query_type & query, value_type const & value, fun_type fun, std::true_type) {
    static_assert(is_composite, "");
    auto const found = query.lower_bound(value);
    if (found.second) {
        /*query.scan_next(found.first, [](record const & p){
            return true;
        });
        query.scan_prev(found.first, [](record const & p){
            return true;
        });*/
    }
    return bc::continue_;
}

template<class this_table, class _record> template<class expr_type, class fun_type, class T> inline break_or_continue
make_query<this_table, _record>::seek_table::scan_if(query_type & query, expr_type const * const expr, fun_type fun, identity<T>, condition_t<condition::WHERE>) {    
    return scan_where(query, static_cast<typename T::col::val_type const &>(expr->value.values), fun);
}

template<class this_table, class _record> template<class expr_type, class fun_type, class T> inline break_or_continue
make_query<this_table, _record>::seek_table::scan_if(query_type & query, expr_type const * const expr, fun_type fun, identity<T>, condition_t<condition::IN>) {
    for (auto & v : expr->value.values) {
        if (bc::break_ == scan_where(query, v, fun)) {
            return bc::break_;
        }
    }
    return bc::continue_;
}

template<class this_table, class _record> 
template<class expr_type, class fun_type, class less_type> break_or_continue
make_query<this_table, _record>::seek_table::scan_less(query_type & query, expr_type const * const expr, fun_type fun, identity<less_type>)
{
    break_or_continue result = bc::continue_;
    query.scan_if([&result, &fun, expr](record const & p){
        if (less_type::apply(p, expr->value.values)) {
            if (fun(p) == bc::break_) {
                result = bc::break_;
                return false;
            }
            return true;
        }
        return false;
    });
    return result;
}

template<class this_table, class _record> 
template<class expr_type, class fun_type, class T> inline break_or_continue
make_query<this_table, _record>::seek_table::scan_if(query_type & query, expr_type const * const expr, fun_type fun, identity<T>, condition_t<condition::LESS>)
{
    using less_type = is_less<typename T::col, sortorder::ASC>;
    return scan_less(query, expr, fun, identity<less_type>{});
}

template<class this_table, class _record> 
template<class expr_type, class fun_type, class T> inline break_or_continue
make_query<this_table, _record>::seek_table::scan_if(query_type & query, expr_type const * const expr, fun_type fun, identity<T>, condition_t<condition::LESS_EQ>)
{
    using less_type = is_less_eq<typename T::col, sortorder::ASC>;
    return scan_less(query, expr, fun, identity<less_type>{});
}

template<class this_table, class _record> 
template<class expr_type, class fun_type, class T> inline break_or_continue
make_query<this_table, _record>::seek_table::scan_if(query_type & query, expr_type const * const expr, fun_type fun, identity<T>, condition_t<condition::GREATER>) {
    return bc::continue_;
}

template<class this_table, class _record> 
template<class expr_type, class fun_type, class T> inline break_or_continue
make_query<this_table, _record>::seek_table::scan_if(query_type & query, expr_type const * const expr, fun_type fun, identity<T>, condition_t<condition::GREATER_EQ>) {
    return bc::continue_;
}

template<class this_table, class _record> 
template<class expr_type, class fun_type, class T> inline break_or_continue
make_query<this_table, _record>::seek_table::scan_if(query_type & query, expr_type const * const expr, fun_type fun, identity<T>, condition_t<condition::BETWEEN>) {
    //return query.scan_with_index(expr->value.values.first, fun,
      //  is_between<typename query_type::T0_col, sortorder::ASC>(expr->value.values.second));
    return bc::continue_;
}

//////////////////////////////////////////////////////////////////////////////////////////

namespace make_query_ {

template<class query_type, class sub_expr_type, bool is_limit>
class SEEK_TABLE final : noncopyable {

    using record_range = typename query_type::record_range;
    using record = typename query_type::record;
    using key_type = typename query_type::key_type;

    using SEARCH = typename SELECT_SEARCH_TYPE<sub_expr_type>::Result;
    using KEYS = SEARCH_KEY<sub_expr_type>;

    static_assert(TL::Length<typename KEYS::key_OR_0>::value || TL::Length<typename KEYS::key_AND_0>::value, "SEEK_TABLE");
    static_assert(!TL::Length<typename KEYS::no_key_OR_0>::value || TL::Length<typename KEYS::key_AND_0>::value, "SEEK_TABLE");

    template<class expr_type, class T>
    bool seek_with_index(expr_type const * const expr, identity<T>);
    
    struct seek_with_index_t {        
        SEEK_TABLE * const m_this;
        explicit seek_with_index_t(SEEK_TABLE * p) : m_this(p){}
        template<class T> 
        bool operator()(identity<T>) const { // T = SEARCH_WHERE
            static_assert(T::cond < condition::lambda, "");
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

    SEEK_TABLE(record_range & result, query_type & query, sub_expr_type const & expr, size_t lim)
        : m_result(result)
        , m_query(query)
        , m_expr(expr)
        , m_limit(lim)
    {
        SDL_ASSERT(is_limit == (m_limit > 0));
        static_assert(IS_SEEK_TABLE<sub_expr_type>::value, "SEEK_TABLE");
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

template<class query_type, class sub_expr_type, bool is_limit> break_or_continue
SEEK_TABLE<query_type, sub_expr_type, is_limit>::push_unique(record const & p)
{
    A_STATIC_ASSERT_NOT_TYPE(void, typename key_type::this_clustered);

    const bool empty = m_result.empty();
    m_result.push_back(p); 

    if (!empty) { // https://en.wikipedia.org/wiki/Insertion_sort
        size_t last = m_result.size() - 1;
        const key_type last_key = query_type::read_key(m_result[last]);
        while (last) {
            auto & prev = m_result[last - 1];
            key_type prev_key = query_type::read_key(prev);
            if (last_key < prev_key) {
                std::swap(m_result[last], prev);
                --last;
            }
            else if (last_key == prev_key) { // found duplicate
                m_result.erase(m_result.begin() + last);
                return bc::continue_;
            }
            else {
                SDL_ASSERT(prev_key < last_key);
                break;
            }
        }
    }
    return has_limit(bool_constant<is_limit>{}) ? bc::break_ : bc::continue_;
}

template<class query_type, class sub_expr_type, bool is_limit>
template<class expr_type, class T> // T = SEARCH_WHERE
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
    using keylist = Select_t<TL::Length<typename KEYS::key_AND_0>::value, 
                                typename KEYS::key_AND_0, 
                                typename KEYS::key_OR_0>;
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

#if SDL_DEBUG_QUERY
    IS_SEEK_TABLE<sub_expr_type>::trace();
#endif
    Select_t<IS_SEEK_TABLE<sub_expr_type>::value, Seek, Scan>(result, query, expr, limit(expr)).select();
}

//--------------------------------------------------------------

template<class sub_expr_type, class TOP, class ORDER>
struct QUERY_VALUES
{
    static_assert(TL::Length<TOP>::value == 1, "TOP");
    static_assert(TL::Length<ORDER>::value, "ORDER");

    template<class record_range, class query_type> static
    void select(record_range & result, query_type & query, sub_expr_type const & expr) {
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

    if (1) {
        SDL_TRACE_QUERY("\nVALUES:");
        where_::trace_::trace_sub_expr(expr);
    }
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
