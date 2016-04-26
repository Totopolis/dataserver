// maketable.h
//
#pragma once
#ifndef __SDL_SYSTEM_MAKETABLE_H__
#define __SDL_SYSTEM_MAKETABLE_H__

#include "maketable_base.h"
#include "index_tree_t.h"

#define maketable_select_old_code   0

#if defined(SDL_OS_WIN32)
#pragma warning(disable: 4503) //decorated name length exceeded, name was truncated
#endif

namespace sdl { namespace db { namespace make {

namespace maketable_ { // protection from unintended ADL

template<class key_type>
inline bool operator < (key_type const & x, key_type const & y) {
    return key_type::this_clustered::is_less(x, y);
}

template<class key_type, class T = typename key_type::this_clustered>
inline bool operator == (key_type const & x, key_type const & y) {
    A_STATIC_ASSERT_NOT_TYPE(void, T);
    return !((x < y) || (y < x));
}
template<class key_type, class T = typename key_type::this_clustered>
inline bool operator != (key_type const & x, key_type const & y) {
    A_STATIC_ASSERT_NOT_TYPE(void, T);
    return !(x == y);
}

} // maketable_

using namespace maketable_;

#if maketable_select_old_code
struct ignore_index {};
struct use_index {};
struct unique_false {};
struct unique_true {};

template<typename col_type> 
struct where {
    using col = col_type;
    using val_type = typename col_type::val_type;
    static const char * name() { return col::name(); }
    val_type const & value;
    where(val_type const & v): value(v) {}
};
#endif

namespace where_ {

enum class condition {
    WHERE, IN, NOT, LESS, GREATER, LESS_EQ, GREATER_EQ, BETWEEN,
    _lambda, _order };

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
inline const char * name(condition_t<condition::_lambda>)       { return "lambda"; }
inline const char * name(condition_t<condition::_order>)        { return "order"; }

template <condition value>
inline const char * condition_name() {
    return name(condition_t<value>());
}

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

template<typename T> struct search_value;
template<typename T>
struct search_value {
    using val_type = T;
    using vector = std::vector<T>;
    vector values;
    search_value(std::initializer_list<val_type> in) : values(in) {}
    search_value(search_value && src) : values(std::move(src.values)) {}
};

template<class val_type>
struct data_type {
private:
    using _Elem = typename std::remove_extent<val_type>::type;
    enum { array_size = array_info<val_type>::size };
public:
    val_type val;
    data_type(const val_type & in) {
        memcpy_pod(val, in);
        static_assert(array_size, "");
    }
    template <typename _Elem, size_t N>
    data_type(_Elem const(&in)[N]) {
        static_assert(N <= array_size, "");
        A_STATIC_ASSERT_TYPE(_Elem[array_size], val_type);
        A_STATIC_ASSERT_IS_POD(_Elem);
        memcpy(&val, &in, sizeof(in));
        if (N < array_size) {
            val[N] = _Elem{};
        }
    }
};

template <typename T, size_t N>
struct search_value<T[N]> {
    using val_type = T[N];    
    using vector = std::vector<data_type<val_type>>;
    vector values;
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
};

template <typename T> inline
std::ostream & trace(std::ostream & out, data_type<T> const & d) {
    out << d.val << " (" << typeid(T).name() << ")";
    return out;
}

template <typename T> inline
std::ostream & trace(std::ostream & out, T const & d) {
    out << d;
    return out;
}

template<condition _c, class T, bool is_array, INDEX>
struct SEARCH;

template<condition _c, class T, INDEX _h> // T = col::
struct SEARCH<_c, T, false, _h> {
    using value_type = search_value<typename T::val_type>;
    static const condition cond = _c;
    static const INDEX hint = _h;
    using col = T;
    value_type value;
    SEARCH(std::initializer_list<typename T::val_type> in): value(in) {
        static_assert(!T::is_array, "!is_array");
    }
};

template<condition _c, class T, INDEX _h> // T = col::
struct SEARCH<_c, T, true, _h> {
private:
    using array_type = typename T::val_type;
    using elem_type = typename std::remove_extent<array_type>::type;
    enum { array_size = array_info<array_type>::size };
public:
    using value_type = search_value<array_type>;
    static const condition cond = _c;
    static const INDEX hint = _h;
    using col = T;
    value_type value;
    template<typename... Args>
    SEARCH(Args const &... args): value(args...) {
        static_assert(T::is_array, "is_array");
        static_assert(array_size, "");
    }
};

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
    static const condition cond = condition::_lambda;
    using value_type = F;
    value_type value;
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
    static const condition cond = condition::_order;
    using col = T;
    static const sortorder value = ord;
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

namespace OL {

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

} //namespace OL

//-------------------------------------------------------------------

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
struct type_at<std::pair<first_type, second_type>, 0>
{
    using Result = first_type;
};

template <class first_type, class second_type, size_t i>
struct type_at<std::pair<first_type, second_type>, i>
{
    using Result = typename type_at<second_type, i-1>::Result;
};

//-------------------------------------------------------------------

template<class pair_type, class T>
struct append_pair;

template<class T>
struct append_pair<NullType, T> {
    using type = std::pair<T, NullType>;
    static type make(NullType end, T && p) {
        A_STATIC_ASSERT_NOT_TYPE(NullType, T);
        return { std::move(p), end };
    }
};

template<class first_type, class second_type, class T>
struct append_pair<std::pair<first_type, second_type>, T> {
private:
    using tail = append_pair<second_type, T>;
public:
    using type = std::pair<first_type, typename tail::type>;
    static type make(std::pair<first_type, second_type> && p1, T && p2) {
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

//-------------------------------------------------------------------

namespace trace_ {

struct trace_operator {
    size_t & count;
    explicit trace_operator(size_t * p) : count(*p){}
    template<operator_ T> 
    void operator()(operator_list<T>) {
        static_assert((T == operator_::OR) || (T == operator_::AND), "");
        SDL_TRACE(++count, ":", (T == operator_::OR) ? "OR" : "AND" );
    }
};

template<class TList> 
inline void trace_operator_list() {
    size_t count = 0;
    OL::operator_processor<TList>::apply(trace_operator(&count));
}

struct trace_SEARCH {
    size_t & count;
    explicit trace_SEARCH(size_t * p) : count(*p){}

    template<condition _c, class T, INDEX _h> // T = col::
    void operator()(identity<SEARCH<_c, T, T::is_array, _h>>) {
        const char * const col_name = T::name();// typeid(T).name();
        const char * const val_name = typeid(typename T::val_type).name();
        SDL_TRACE(++count, ":", condition_name<_c>(), "<", col_name, ">", " (", val_name, ")",
            " INDEX::", index_name<_h>());
    }
    template<class T, sortorder ord> 
    void operator()(identity<ORDER_BY<T, ord>>) {
        SDL_TRACE(++count, ":ORDER_BY<", T::name(), "> ", to_string::type_name(ord));
    }
    template<class T>
    void operator()(identity<T>) {
        SDL_TRACE(++count, ":", typeid(T).name());
    }
};

template<class TList> 
inline void trace_search_list() {
    size_t count = 0;
    meta::processor<TList>::apply(trace_SEARCH(&count));
}

struct print_value {
private:
    template<class T>
    static void trace(std::vector<T> const & vec) {
        std::cout << typeid(T).name() << " = ";
        size_t i = 0;
        for (auto & it : vec) {
            if (i++) std::cout << ",";
            where_::trace(std::cout, it);
        }
        std::cout << std::endl;
    }
    template<class T> // T = lambda
    static void trace(T const & value) {
        SDL_TRACE(typeid(T).name());
    }
    template<class T> // T = search_value
    static void trace(search_value<T> const & value) {
        trace(value.values);
    }
    static void trace(sortorder const value) {
        SDL_TRACE(to_string::type_name(value));
    }
    size_t & count;
public:
    explicit print_value(size_t * p) : count(*p){}
    template<class T> // T = sub_expr_value 
    void operator()(T const & value) {
        std::cout << (++count) << ":";
        print_value::trace(value.value);
    }
};

template<class T> 
inline void trace_sub_expr(T const & s) {
    trace_search_list<typename T::type_list>();
    trace_operator_list<typename T::oper_list>();
    size_t count = 0;
    processor_pair<typename T::pair_type>::apply(s.value, trace_::print_value(&count));
}

} // trace_
} // where_

namespace select_ {

using operator_ = where_::operator_;

template<class query_type, class TList, class OList, class next_value, class prev_value>
struct sub_expr : noncopyable
{    
    using type_list = TList;
    using oper_list = OList;
    enum { type_size = TL::Length<type_list>::value };
private:
    template<class _SEARCH>
    struct sub_expr_value {
    private:
        using value_t = typename _SEARCH::value_type;
    public:
        value_t value;
        sub_expr_value(_SEARCH const & s) = delete;
        sub_expr_value(_SEARCH && s): value(std::move(s.value)) { // move only
            A_STATIC_ASSERT_TYPE(_SEARCH, next_value);
            A_STATIC_ASSERT_NOT_TYPE(typename value_t::vector, NullType);
            SDL_ASSERT(!this->value.values.empty());
        }
    };
    template<class T, sortorder ord>
    struct sub_expr_value<where_::ORDER_BY<T, ord>> {
    private:
        using param_t = where_::ORDER_BY<T, ord>;
    public:
        using type = T;
        static const sortorder value = ord;
        sub_expr_value(param_t const &) = delete;
        sub_expr_value(param_t &&) {
            A_STATIC_ASSERT_TYPE(param_t, next_value);
        }
    };        
    template<class F>
    struct sub_expr_value<where_::SELECT_IF<F>> {
    private:
        using param_t = where_::SELECT_IF<F>;
        using value_t = typename param_t::value_type;
    public:
        value_t value;
        sub_expr_value(param_t const & s) = delete;
        sub_expr_value(param_t && s): value(std::move(s.value)) {
            A_STATIC_ASSERT_TYPE(param_t, next_value);
        }
    };
public:
    using value_type = sub_expr_value<next_value>;
    using append_pair = where_::append_pair<prev_value, value_type>;
    using pair_type = typename append_pair::type;

    query_type & m_query;
    pair_type value;

    template<size_t i>
    auto get() const -> decltype(where_::get_value<i>::get(value)) {
        static_assert(i < TL::Length<type_list>::value, "");
        return where_::get_value<i>::get(value);
    }
    template<size_t i>
    auto get(Size2Type<i>) const -> decltype(get<i>()) {
        return get<i>();
    }
private:
    template<class T, operator_ OP>
    using ret_expr = sub_expr<
            query_type, 
            typename TL::Append<type_list, sdl::remove_reference_t<T>>::Result,
            typename where_::OL::append<oper_list, OP>::Result,
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
    template<class T> // T = where_::SEARCH
    ret_expr<T, operator_::OR> operator | (T && s) {
        return { m_query, std::forward<T>(s), std::move(this->value) };
    }
    template<class T>
    ret_expr<T, operator_::AND> operator && (T && s) {
        return { m_query, std::forward<T>(s), std::move(this->value) };
    }
    using record_range = typename query_type::record_range;
    record_range VALUES() {
        static_assert(type_size == where_::OL::length<oper_list>::value, "");
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

    template<class T> // T = where_::SEARCH or where_::IF
    ret_expr<T, operator_::OR> operator | (T && s) {
        return { m_query, std::forward<T>(s) };
    }
    template<class T>
    ret_expr<T, operator_::AND> operator && (T && s) {
        return { m_query, std::forward<T>(s) };
    }
};

} // select_ 

template<class this_table, class record>
class make_query: noncopyable {
    using table_clustered = typename this_table::clustered;
    using key_type = meta::cluster_key<table_clustered, NullType>;
    using KEY_TYPE_LIST = meta::cluster_type_list<table_clustered, NullType>;
    enum { index_size = meta::cluster_index_size<table_clustered>::value };
private:
    this_table & m_table;
    page_head const * const m_cluster;
public:
    using record_range = std::vector<record>;
    make_query(this_table * p, database * const d, shared_usertable const & s)
        : m_table(*p)
        , m_cluster(d->get_cluster_root(_schobj_id(this_table::id)))
    {
        SDL_ASSERT(meta::test_clustered<table_clustered>());
    }
    template<class fun_type>
    void scan_if(fun_type fun) {
        for (auto p : m_table) {
            A_STATIC_CHECK_TYPE(record, p);
            if (!fun(p)) {
                break;
            }
        }
    }
    template<class fun_type>
    record find(fun_type fun) {
        for (auto p : m_table) { // linear search
            A_STATIC_CHECK_TYPE(record, p);
            if (fun(p)) {
                return p;
            }
        }
        return {};
    }
    template<typename... Ts>
    record find_with_index_n(Ts&&... params) {
        static_assert(index_size == sizeof...(params), ""); 
        return find_with_index(make_key(params...));
    }
    record find_with_index(key_type const &);
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
        void operator()(identity<T>) const {
            enum { set_index = key_index<T>::value };
            A_STATIC_ASSERT_IS_POD(typename T::type);
            meta::copy(dest.set(Int2Type<set_index>()), src.val(identity<typename T::col>()));
        }
    };
#if maketable_select_old_code
    class select_key_list  {
        const key_type * _First;
        const key_type * _Last;
    public:
        select_key_list(std::initializer_list<key_type> in) : _First(in.begin()), _Last(in.end()) {}
        select_key_list(key_type const & in) : _First(&in), _Last(&in + 1){}
        select_key_list(std::vector<key_type> const & in) : _First(in.data()), _Last(in.data() + in.size()){}
        const key_type * begin() const {
            return _First;
        }
        const key_type * end() const {
            return _Last;
        }
        size_t size() const {
            return ((size_t)(_Last - _First));
        }
    };
#endif
    template<size_t i> static void set_key(key_type &) {}
    template<size_t i, typename T, typename... Ts>
    static void set_key(key_type & dest, T && value, Ts&&... params) {
        dest.set(Int2Type<i>()) = std::forward<T>(value);
        set_key<i+1>(dest, params...);
    }
public:
    static key_type read_key(record const & src) {
        key_type dest; // uninitialized
        meta::processor<KEY_TYPE_LIST>::apply(read_key_fun(dest, src));
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
#if maketable_select_old_code
    record_range select(select_key_list, ignore_index, unique_false);
    record_range select(select_key_list, ignore_index, unique_true);
    record_range select(select_key_list, use_index, unique_true);
    static void select_n() {}
public:
    template<class T1 = use_index, class T2 = unique_true>
    record_range select(select_key_list in) {
        return select(in, T1(), T2());
    }
    //record_range select(select_key_list, enum_index = enum_index::use_index, enum_unique = enum_unique::unique_true);    
    //FIXME: select * from GeoTable as gt where myPoint.STDistance(gt.Geo) <= 50

    template<typename T, typename... Ts> 
    void select_n(where<T> col, Ts const & ... params) {
        enum { col_found = TL::IndexOf<typename this_table::type_list, T>::value };
        enum { key_found = meta::cluster_col_find<KEY_TYPE_LIST, T>::value };
        static_assert(col_found != -1, "");
        using type_list = typename TL::Seq<T, Ts...>::Type; // test
        static_assert(TL::Length<type_list>::value == sizeof...(params) + 1, "");
        SDL_ASSERT(where<T>::name() == T::name()); // same memory
        SDL_TRACE(
            "col:", col_found, 
            " key:", key_found, 
            " name:", T::name(),
            " value:", col.value);
        select_n(params...);
    }
#endif
private:
    using select_expr = select_::select_expr<make_query>;
public:
    template<class sub_expr_type>
    record_range VALUES(sub_expr_type const & expr);
public:
    select_expr SELECT { this };
};

} // make
} // db
} // sdl

#include "maketable.hpp"

#endif // __SDL_SYSTEM_MAKETABLE_H__
