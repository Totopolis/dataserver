// maketable_where.h
//
#pragma once
#ifndef __SDL_SYSTEM_MAKETABLE_WHERE_H__
#define __SDL_SYSTEM_MAKETABLE_WHERE_H__

#if defined(SDL_OS_WIN32)
#pragma warning(disable: 4503) //decorated name length exceeded, name was truncated
#endif

#include "spatial/spatial_type.h"

namespace sdl { namespace db { namespace make {
namespace where_ {

enum class condition {
    WHERE,          // 0
    IN,             // 1
    NOT,            // 2
    LESS,           // 3
    GREATER,        // 4
    LESS_EQ,        // 5
    GREATER_EQ,     // 6
    BETWEEN,        // 7
    lambda,         // 8
    ORDER_BY,       // 9
    TOP,            // 10
    STContains,     // 11
    STIntersects,   // 12
    STDistance,     // 13
    _end
};

template<condition T> 
using condition_t = Val2Type<condition, T>;

inline const char * name(condition_t<condition::WHERE>)         { return "WHERE"; }
inline const char * name(condition_t<condition::IN>)            { return "IN"; }
inline const char * name(condition_t<condition::NOT>)           { return "NOT"; }
inline const char * name(condition_t<condition::LESS>)          { return "LESS"; }
inline const char * name(condition_t<condition::GREATER>)       { return "GREATER"; }
inline const char * name(condition_t<condition::LESS_EQ>)       { return "LESS_EQ"; }
inline const char * name(condition_t<condition::GREATER_EQ>)    { return "GREATER_EQ"; }
inline const char * name(condition_t<condition::BETWEEN>)       { return "BETWEEN"; }
inline const char * name(condition_t<condition::lambda>)        { return "lambda"; }
inline const char * name(condition_t<condition::ORDER_BY>)      { return "ORDER_BY"; }
inline const char * name(condition_t<condition::TOP>)           { return "TOP"; }
inline const char * name(condition_t<condition::STContains>)    { return "STContains"; }
inline const char * name(condition_t<condition::STIntersects>)  { return "STIntersects"; }
inline const char * name(condition_t<condition::STDistance>)    { return "STDistance"; }

template <condition value>
inline const char * condition_name() {
    return name(condition_t<value>());
}

template <condition c1, condition c2>
struct is_condition
{
    enum { value = (c1 == c2) };
};

template <condition c1, condition c2>
struct not_condition
{
    enum { value = (c1 != c2) };
};

template <condition c>
using is_condition_top = is_condition<condition::TOP, c>;

template <condition c>
using is_condition_order = is_condition<condition::ORDER_BY, c>;

template <condition c>
using is_condition_lambda = is_condition<condition::lambda, c>;

template <condition c>
struct is_condition_search {
    enum { value = (c <= condition::lambda) };
};

template <> 
struct is_condition_search<condition::STContains> {
    enum { value = true };
};

template <condition c>
struct is_condition_index {
    enum { value = (c < condition::lambda) && (c != condition::NOT) };
};

template <condition c>
struct has_index_hint {
    enum { value = (c < condition::lambda) };
};

//------------------------------------------------------------------------

enum class INDEX { AUTO, USE, IGNORE }; // index hint

template<INDEX T>
using INDEX_t = Val2Type<INDEX, T>;

inline const char * name(INDEX_t<INDEX::AUTO>)    { return "AUTO"; }
inline const char * name(INDEX_t<INDEX::USE>)     { return "USE"; }
inline const char * name(INDEX_t<INDEX::IGNORE>)  { return "IGNORE"; }

template <INDEX value>
inline const char * index_name() {
    return name(INDEX_t<value>());
}

//---------------------------------------------------------------

template<class T1, class T2>
struct pair_t
{
    typedef T1 first_type;
    typedef T2 second_type;

    T1 first;
    T2 second;

    pair_t(T1 && v1, T2 && v2)
        : first(std::move(v1))
        , second(std::move(v2))
    {
        A_STATIC_ASSERT_NOT_TYPE(T1, NullType);
    }
    pair_t(T1 && v1)
        : first(std::move(v1))
    {
        A_STATIC_ASSERT_NOT_TYPE(T1, NullType);
        A_STATIC_CHECK_TYPE(NullType, second);
    }
    pair_t(pair_t && src)
        : first(std::move(src.first))
        , second(std::move(src.second))
    {
        A_STATIC_ASSERT_NOT_TYPE(T1, NullType);
    }
};

//---------------------------------------------------------------

template<class val_type>
struct array_value {
private:
    using _Elem = typename std::remove_extent<val_type>::type;
    enum { length = array_info<val_type>::size };
public:
    val_type val;
    array_value(const val_type & in) {
        memcpy_pod(val, in);
        static_assert(length, "");
    }
    template <typename _Elem, size_t N>
    array_value(_Elem const(&in)[N]) {
        static_assert(N <= length, "");
        A_STATIC_ASSERT_TYPE(_Elem[length], val_type);
        A_STATIC_ASSERT_IS_POD(_Elem);
        memcpy(&val, &in, sizeof(in));
        if (N < length) {
            val[N] = _Elem{};
        }
    }
    operator val_type const & () const {
        return val;
    }
};

template <typename T> inline
std::ostream & trace(std::ostream & out, array_value<T> const & d) {
    out << to_string::type(d.val) << " (" << typeid(T).name() << ")";
    return out;
}

template <typename T> inline
std::ostream & trace(std::ostream & out, T const & d) {
    out << d;
    return out;
}

//---------------------------------------------------------------

enum class dim { vector, _1, _2 };

template<condition> struct condition_dim {
    static const dim value = dim::_1;
};
template<> struct condition_dim<condition::BETWEEN> {
    static const dim value = dim::_2;
};
template<> struct condition_dim<condition::IN> {
    static const dim value = dim::vector;
};
template<> struct condition_dim<condition::NOT> {
    static const dim value = dim::vector;
};

//---------------------------------------------------------------

template<condition cond, typename T, dim = condition_dim<cond>::value>
struct search_value;

template<condition cond, typename T>
struct search_value<cond, T, dim::vector> {
    using val_type = T;
    using values_t = std::vector<T>;
    values_t values;
    search_value(std::initializer_list<val_type> in) : values(in) {}
    search_value(search_value && src) : values(std::move(src.values)) {}    
    bool empty() const { 
        return values.empty();
    }
};

template<condition cond, typename T>
struct search_value<cond, T, dim::_1> {
    using val_type = T;
    using values_t = T;
    values_t values;
    search_value(search_value && src) : values(std::move(src.values)) {}    
    search_value(val_type && v1)
        : values(std::move(v1))
    {}
    static bool empty() { return false; }
};

template<condition cond, typename T>
struct search_value<cond, T, dim::_2> {
    using val_type = T;
    using values_t = pair_t<T, T>;
    values_t values;
    search_value(search_value && src) : values(std::move(src.values)) {}    
    search_value(val_type && v1, val_type && v2)
        : values(std::move(v1), std::move(v2))
    {}
    static bool empty() { return false; }
};

//---------------------------------------------------------------

template<condition cond, typename T, size_t N>
struct search_value<cond, T[N], dim::vector> {
    using val_type = T[N];    
    using values_t = std::vector<array_value<val_type>>;
    values_t values;
    search_value(val_type const & in): values(1, in) {}
private:
    static void push_back() {}
    template<typename Arg1, typename... Args>
    void push_back(Arg1 const & arg1, Args const &... args) {
        values.push_back(arg1);
        push_back(args...);
    }
public:
    template<typename... Args>
    explicit search_value(Args const &... args) {
        static_assert(sizeof...(args), "");
        values.reserve(sizeof...(args));
        push_back(args...);
    }
    bool empty() const { 
        return values.empty();
    }
};

template<condition cond, typename T, size_t N>
struct search_value<cond, T[N], dim::_1> {
    using val_type = T[N];    
    using values_t = array_value<val_type>;
    values_t values;
    template<typename Arg1>
    search_value(Arg1 const & v1): values(v1) {}
    static bool empty() { return false; }
};

template<condition cond, typename T, size_t N>
struct search_value<cond, T[N], dim::_2> {
    using val_type = T[N];    
    using values_t = pair_t<array_value<val_type>, array_value<val_type>>;
    values_t values;
    template<typename Arg1, typename Arg2>
    search_value(Arg1 const & v1, Arg2 const & v2)
        : values(v1, v2)
    {}
    static bool empty() { return false; }
};

//---------------------------------------------------------------

template<condition cond, class T, bool is_array, INDEX, dim = condition_dim<cond>::value>
struct SEARCH;

template<condition _c, class T, INDEX _h> // T = col::
struct SEARCH<_c, T, false, _h, dim::vector> {
private:
    using col_val = typename T::val_type;
public:
    static const condition cond = _c;
    static const INDEX hint = _h;
    using col = T;
    using value_type = search_value<cond, col_val, dim::vector>;
    value_type value;
    SEARCH(std::initializer_list<col_val> in): value(in) {
        static_assert(!T::is_array, "!is_array");
    }
};

template<condition _c, class T, INDEX _h> // T = col::
struct SEARCH<_c, T, false, _h, dim::_1> {
private:
    using col_val = typename T::val_type;
public:
    static const condition cond = _c;
    static const INDEX hint = _h;
    using col = T;
    using value_type = search_value<cond, col_val, dim::_1>;
    value_type value;
    SEARCH(col_val && v1): value(std::move(v1)) {
        static_assert(!T::is_array, "!is_array");
    }
};

template<condition _c, class T, INDEX _h> // T = col::
struct SEARCH<_c, T, false, _h, dim::_2> {
private:
    using col_val = typename T::val_type;
public:
    static const condition cond = _c;
    static const INDEX hint = _h;
    using col = T;
    using value_type = search_value<cond, col_val, dim::_2>;
    value_type value;
    SEARCH(col_val && v1, col_val && v2)
        : value(std::move(v1), std::move(v2)) {
        static_assert(!T::is_array, "!is_array");
#if SDL_DEBUG
        SDL_ASSERT_1(!meta::col_less<T, sortorder::ASC>::less(value.values.second, value.values.first));
#endif
    }
};

template<condition _c, class T, INDEX _h, dim _d> // T = col::
struct SEARCH<_c, T, true, _h, _d> {
private:
    using array_type = typename T::val_type;
    using elem_type = typename std::remove_extent<array_type>::type;
    enum { array_size = array_info<array_type>::size };
public:
    static const condition cond = _c;
    static const INDEX hint = _h;
    using col = T;
    using value_type = search_value<cond, array_type, _d>;
    value_type value;
    template<typename... Args>
    SEARCH(Args const &... args): value(args...) {
        static_assert(T::is_array, "is_array");
        static_assert(array_size, "");
        static_assert(sizeof...(args), "");
        static_assert((cond != condition::WHERE) || (sizeof...(args) == 1), "WHERE");
        static_assert((cond != condition::BETWEEN) || (sizeof...(args) == 2), "BETWEEN");
    }
};

//---------------------------------------------------------------

template<class T, INDEX h = INDEX::AUTO> using WHERE       = SEARCH<condition::WHERE,      T, T::is_array, h>;
template<class T, INDEX h = INDEX::AUTO> using IN          = SEARCH<condition::IN,         T, T::is_array, h>;
template<class T, INDEX h = INDEX::AUTO> using NOT         = SEARCH<condition::NOT,        T, T::is_array, h>;
template<class T, INDEX h = INDEX::AUTO> using LESS        = SEARCH<condition::LESS,       T, T::is_array, h>;
template<class T, INDEX h = INDEX::AUTO> using GREATER     = SEARCH<condition::GREATER,    T, T::is_array, h>;
template<class T, INDEX h = INDEX::AUTO> using LESS_EQ     = SEARCH<condition::LESS_EQ,    T, T::is_array, h>;
template<class T, INDEX h = INDEX::AUTO> using GREATER_EQ  = SEARCH<condition::GREATER_EQ, T, T::is_array, h>;
template<class T, INDEX h = INDEX::AUTO> using BETWEEN     = SEARCH<condition::BETWEEN,    T, T::is_array, h>;

template<class F>
struct SELECT_IF {
    static const condition cond = condition::lambda;
    using value_type = F;
    value_type value;
    using col = void;
    SELECT_IF(value_type f) : value(f){}
    template<class record>
    bool operator()(record p) {
        return value(p);
    }
};

template<class fun_type> inline
SELECT_IF<fun_type> IF(fun_type f) {
    return SELECT_IF<fun_type>(f);
}

template<class T, sortorder ord = sortorder::ASC> // T = col::
struct ORDER_BY {
    static_assert(ord != sortorder::NONE, "ORDER_BY");
    static const condition cond = condition::ORDER_BY;
    using col = T;
    static const sortorder value = ord;
    enum { _order = (int)ord };  // workaround for error C2057: expected constant expression (VS 2015)
#if 0 //defined(SDL_OS_WIN32) && (_MSC_VER == 1800) // VS 2013
    // workaround for fatal error C1001: An internal error has occurred in the compiler
    //(compiler file 'f:\dd\vctools\compiler\utc\src\p2\ehexcept.c', line 956)
    ORDER_BY(std::initializer_list<int> tmp) {
        SDL_ASSERT(!tmp.size());
    }
#else
    ORDER_BY() = default; // require: && ORDER_BY (VS 2013)
#endif
};

struct TOP {
    static const condition cond = condition::TOP;
    using col = void;
    using value_type = size_t;
    value_type value;
    TOP(value_type v) : value(v){}
};

//-------------------------------------------------------------------
#if 0
template<condition _c, class T, INDEX _h> // T = col::
struct SPATIAL {
    static const condition cond = _c;
    static const INDEX hint = _h;
    using col = T;
    using value_type = spatial_point;
    value_type value;
};
template<class T, INDEX h = INDEX::AUTO> using STContains     = SPATIAL<condition::STContains, T, h>;
template<class T, INDEX h = INDEX::AUTO> using STIntersects   = SPATIAL<condition::STIntersects, T, h>;
template<class T, INDEX h = INDEX::AUTO> using STDistance     = SPATIAL<condition::STDistance, T, h>;
#endif

template<class T, INDEX _h = INDEX::AUTO> // T = col::
struct STContains {
    static_assert(T::type == scalartype::t_geography, "");
    static const condition cond = condition::STContains;
    static const INDEX hint = _h;
    using col = T;
    using value_type = search_value<cond, spatial_point, dim::_1>;
    value_type value;
    STContains(spatial_point p): value(std::move(p)){}
    STContains(Latitude lat, Longitude lon)
        : value(spatial_point::init(lat, lon)){}
};

//-------------------------------------------------------------------

enum class operator_ { OR, AND };

template <operator_ T, class U = NullType>
struct operator_list {
    static const operator_ Head = T;
    typedef U Tail;
};

template<operator_ T>
using operator_t = Val2Type<operator_, T>;

inline const char * name(operator_t<operator_::OR>)    { return "OR"; }
inline const char * name(operator_t<operator_::AND>)   { return "AND"; }

template <operator_ value>
inline const char * operator_name() {
    return name(operator_t<value>());
}

//-------------------------------------------------------------------

namespace oper_ {

template <class TList> struct length;
template <> struct length<NullType>
{
    enum { value = 0 };
};

template <operator_ T, class U>
struct length< operator_list<T, U> >
{
    enum { value = 1 + length<U>::value };
};

//-------------------------------------------------------------------

template <class TList, operator_ T> struct append;

template <operator_ T> 
struct append<NullType, T>
{
    typedef operator_list<T> Result;
};

template <operator_ Head, class Tail, operator_ T>
struct append<operator_list<Head, Tail>, T>
{
    typedef operator_list<Head, typename append<Tail, T>::Result> Result;
};

//-------------------------------------------------------------------

template <class TList> struct reverse;

template <> struct reverse<NullType>
{
    typedef NullType Result;
};

template <operator_ Head, class Tail>
struct reverse< operator_list<Head, Tail> >
{
    typedef typename append<
        typename reverse<Tail>::Result, Head>::Result Result;
};

//-------------------------------------------------------------------

template<class TList> struct operator_processor;

template<> struct operator_processor<NullType> {
    template<class fun_type>
    static void apply(fun_type){}
};
template <operator_ T, class U>
struct operator_processor<operator_list<T, U>> {
    template<class fun_type>
    static void apply(fun_type fun){
        fun(operator_list<T>());
        operator_processor<U>::apply(fun);
    }
};

} // oper_

namespace pair_ {

template<class T> struct processor_pair;
template<> struct processor_pair<NullType>
{
    template<class fun_type>
    static void apply(NullType, fun_type){}
};

template <class pair_type>
struct processor_pair
{
    template<class fun_type>
    static void apply(pair_type const & value, fun_type fun){
        fun(value.first);
        processor_pair<typename pair_type::second_type>::apply(value.second, fun);
    }
};

//-------------------------------------------------------------------

template<class pair_type, size_t index>
struct type_at;

template <class first_type, class second_type>
struct type_at<pair_t<first_type, second_type>, 0>
{
    using Result = first_type;
};

template <class first_type, class second_type, size_t i>
struct type_at<pair_t<first_type, second_type>, i>
{
    using Result = typename type_at<second_type, i-1>::Result;
};

//-------------------------------------------------------------------

template<class pair_type, class T>
struct append_pair;

template<class T>
struct append_pair<NullType, T> {
    using type = pair_t<T, NullType>;
    static inline type make(NullType, T && p) {
        A_STATIC_ASSERT_TYPE(typename std::remove_cv<T>::type, T);
        A_STATIC_ASSERT_NOT_TYPE(NullType, T);
        return type{ std::move(p) };
    }
};

template<class first_type, class second_type, class T>
struct append_pair<pair_t<first_type, second_type>, T> {
private:
    using tail = append_pair<second_type, T>;
public:
    using type = pair_t<first_type, typename tail::type>;
    static inline type make(pair_t<first_type, second_type> && p1, T && p2) {
        A_STATIC_ASSERT_TYPE(typename std::remove_cv<T>::type, T);
        return type{ 
            std::move(p1.first), 
            tail::make(std::move(p1.second), std::move(p2))
        };
    }
};

//-------------------------------------------------------------------

template <size_t i> struct get_value;

template<> struct get_value<0>
{
    template <class pair_type> static
    typename pair_type::first_type const *
    get(pair_type const & value) {
        return &(value.first);
    }
};

template<size_t i> 
struct get_value
{
    template <class pair_type> static 
    typename type_at<pair_type, i>::Result const *
    get(pair_type const & value) {
        return get_value<i-1>::get(value.second);
    }
};

} // pair_ 

namespace trace_ {

struct trace_operator {
    size_t & count;
    explicit trace_operator(size_t * p) : count(*p){}
    template<operator_ T> 
    void operator()(operator_list<T>) {
        static_assert((T == operator_::OR) || (T == operator_::AND), "");
        SDL_TRACE(count++, ":", (T == operator_::OR) ? "OR" : "AND" );
    }
};

template<class TList> 
inline void trace_operator_list() {
    size_t count = 0;
    oper_::operator_processor<TList>::apply(trace_operator(&count));
}

struct trace_SEARCH {
    size_t & count;
    explicit trace_SEARCH(size_t * p) : count(*p){}

    template<condition _c, class T, INDEX _h> // T = col::
    bool operator()(identity<SEARCH<_c, T, T::is_array, _h>>) {
        const char * const col_name = T::name();
        const char * const val_name = typeid(typename T::val_type).name();
        SDL_TRACE(count++, ":", condition_name<_c>(), "<", col_name, ">", " (", val_name, ")", " INDEX::", index_name<_h>());
        return true;
    }
    template<class T, sortorder ord> 
    bool operator()(identity<ORDER_BY<T, ord>>) {
        SDL_TRACE(count++, ":ORDER_BY<", T::name(), "> ", to_string::type_name(ord));
        return true;
    }
    template<class T>
    bool operator()(identity<T>) {
        SDL_TRACE(count++, ":",  condition_name<T::cond>(), " = ", typeid(T).name());
        return true;
    }
};

template<class TList> 
inline void trace_search_list() {
    size_t count = 0;
    meta::processor_if<TList>::apply(trace_SEARCH(&count));
}

struct print_value {
private:
    template<class T, condition cond>
    static void trace(std::vector<T> const & vec, condition_t<cond>) {
        std::cout << typeid(T).name() << " = ";
        size_t i = 0;
        for (auto & it : vec) {
            if (i++) std::cout << ",";
            where_::trace(std::cout, it);
        }
        std::cout << std::endl;
    }
    template<class T, condition cond>
    static void trace(T const & value, condition_t<cond>) {
        (void)value;
        SDL_TRACE(condition_name<cond>(), " = ", typeid(T).name());
    }
    template<class T, condition cond>
    static void trace(search_value<cond, T> const & value, condition_t<cond> c) {
        trace(value.values, c);
    }
    template<condition cond>
    static void trace(sortorder const value, condition_t<cond>) {
        SDL_TRACE(condition_name<cond>(), " = ", to_string::type_name(value));
    }
    size_t & count;
public:
    explicit print_value(size_t * p) : count(*p){}
    template<class T> // T = sub_expr_value 
    void operator()(T const & value) {
        (void)value;
        std::cout << (count++) << ":";
        print_value::trace(value.value, condition_t<T::cond>{});
    }
};

template<class T> 
inline void trace_sub_expr(T const & s) {
    trace_search_list<typename T::type_list>();
    trace_operator_list<typename T::oper_list>();
    size_t count = 0;
    pair_::processor_pair<typename T::pair_type>::apply(s.value, trace_::print_value(&count));
}

} // trace_
} // where_

namespace select_ {

using operator_ = where_::operator_;
using condition = where_::condition;

template<class _SEARCH>
struct sub_expr_value {
private:
    using value_t = typename _SEARCH::value_type;
public:
    static const condition cond = _SEARCH::cond;
    value_t value;
    sub_expr_value(_SEARCH const & s) = delete;
    sub_expr_value(_SEARCH && s): value(std::move(s.value)) { // move only
        A_STATIC_ASSERT_NOT_TYPE(typename value_t::values_t, NullType);
        SDL_ASSERT(!this->value.empty());
    }
};

template<class T, sortorder ord>
struct sub_expr_value<where_::ORDER_BY<T, ord>> {
private:
    using param_t = where_::ORDER_BY<T, ord>;
public:
    static const condition cond = param_t::cond;
    using type = T;
    static const sortorder value = ord;
    sub_expr_value(param_t const &) = delete;
    sub_expr_value(param_t &&) {}
};        

template<class F>
struct sub_expr_value<where_::SELECT_IF<F>> {
private:
    using param_t = where_::SELECT_IF<F>;
    using value_t = typename param_t::value_type;
public:
    static const condition cond = param_t::cond;
    value_t value;
    sub_expr_value(param_t const & s) = delete;
    sub_expr_value(param_t && s): value(std::move(s.value)) {}
};

template<> struct sub_expr_value<where_::TOP> {
private:
    using param_t = where_::TOP;
    using value_t = where_::TOP::value_type;
public:
    static const condition cond = param_t::cond;
    value_t value;
    sub_expr_value(param_t const & s) = delete;
    sub_expr_value(param_t && s): value(std::move(s.value)) {
        A_STATIC_ASSERT_TYPE(size_t, value_t);
    }
};

//------------------------------------------------------------------

template<class query_type, class TList, class OList, class next_value, class prev_value>
struct sub_expr : noncopyable
{    
    using type_list = TList;                            // = Typelist<where_::SEARCH>
    using oper_list = OList;                            // = where_::operator_list
    enum { type_size = TL::Length<type_list>::value };
public:
    using value_type = sub_expr_value<next_value>;
    using append_pair = where_::pair_::append_pair<prev_value, value_type>;
    using pair_type = typename append_pair::type;

    query_type & m_query;
    pair_type value;

    template<size_t i>
    auto get() const -> decltype(where_::pair_::get_value<i>::get(value)) {
        return where_::pair_::get_value<i>::get(value);
    }
    template<size_t i>
    auto get(Size2Type<i>) const -> decltype(where_::pair_::get_value<i>::get(value)) {
        return where_::pair_::get_value<i>::get(value);
    }
    /*template<size_t i>
    auto get(Size2Type<i>) const -> decltype(get<i>()) { gcc compiler error
        return get<i>();
    }*/
private:
    template<class T, operator_ OP>
    using ret_expr = sub_expr<
            query_type, 
            typename TL::Append<type_list, sdl::remove_reference_t<T>>::Result,
            typename where_::oper_::append<oper_list, OP>::Result,
            sdl::remove_reference_t<T>,
            pair_type
    >;
public:
    sub_expr(query_type & q, next_value && s): m_query(q)
        , value(std::move(s), NullType())
    {
        A_STATIC_ASSERT_TYPE(NullType, prev_value);
    }
    sub_expr(query_type & q, next_value && s, prev_value && t): m_query(q)
        , value(append_pair::make(std::move(t), std::move(s)))
    {
        A_STATIC_ASSERT_NOT_TYPE(NullType, prev_value);
    }
public:
    template<class T> // T = where_::SEARCH | where_::IF | where_::TOP
    ret_expr<T, operator_::OR> operator | (T && s) {
        return { m_query, std::forward<T>(s), std::move(this->value) };
    }
    template<class T> // T = where_::ORDER_BY
    ret_expr<T, operator_::AND> operator && (T && s) {
        return { m_query, std::forward<T>(s), std::move(this->value) };
    }
    using record_range = typename query_type::record_range;
    record_range VALUES() {
        static_assert((int)type_size == (int)where_::oper_::length<oper_list>::value, "");
        return m_query.VALUES(*this);
    }
    operator record_range() { 
        return VALUES();
    }
};

template<class query_type>
class select_expr : noncopyable
{   
    using record_range = typename query_type::record_range;
    query_type & m_query;

    template<class T, operator_ OP>
    using ret_expr = sub_expr<
        query_type, 
        Typelist<sdl::remove_reference_t<T>, NullType>,
        where_::operator_list<OP, NullType>,
        sdl::remove_reference_t<T>,
        NullType
    >;
public:
    explicit select_expr(query_type * q): m_query(*q) {}

    template<class T> // T = where_::SEARCH | where_::IF | where_::TOP
    ret_expr<T, operator_::OR> operator | (T && s) {
        return { m_query, std::forward<T>(s) };
    }
    template<class T>
    ret_expr<T, operator_::AND> operator && (T && s) {
        return { m_query, std::forward<T>(s) };
    }
};

} // select_ 
} // make
} // db
} // sdl

#endif // __SDL_SYSTEM_MAKETABLE_WHERE_H__
