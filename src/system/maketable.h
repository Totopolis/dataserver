// maketable.h
//
#pragma once
#ifndef __SDL_SYSTEM_MAKETABLE_H__
#define __SDL_SYSTEM_MAKETABLE_H__

#include "maketable_base.h"
#include "index_tree_t.h"

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

//------------------------------------------------------------------------------

struct ignore_index {}; //FIXME: will be removed
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

namespace where_ { //FIXME: prototype

template<class T, sortorder ord = sortorder::ASC> // T = col::
struct ORDER_BY {
    using col = T;
    static const sortorder value = ord;
};

enum class condition { WHERE, IN, NOT, LESS, GREATER, LESS_EQ, GREATER_EQ, BETWEEN };

inline const char * name(Val2Type<condition, condition::WHERE>)         { return "WHERE"; }
inline const char * name(Val2Type<condition, condition::IN>)            { return "IN"; }
inline const char * name(Val2Type<condition, condition::NOT>)           { return "NOT"; }
inline const char * name(Val2Type<condition, condition::LESS>)          { return "LESS"; }
inline const char * name(Val2Type<condition, condition::GREATER>)       { return "GREATER"; }
inline const char * name(Val2Type<condition, condition::LESS_EQ>)       { return "LESS_EQ"; }
inline const char * name(Val2Type<condition, condition::GREATER_EQ>)    { return "GREATER_EQ"; }
inline const char * name(Val2Type<condition, condition::BETWEEN>)       { return "BETWEEN"; }

template <condition value>
inline const char * condition_name() {
    return name(Val2Type<condition, value>());
}

template<typename T> struct search_value;

template<typename T>
struct search_value {
    using val_type = T;
    using vector = std::vector<T>;
    vector values;
    search_value(std::initializer_list<val_type> in) : values(in) {}
    search_value(search_value && src) : values(std::move(src.values)) {}
    search_value(search_value const & src) : values(src.values) {}
};

template <typename T, size_t N>
struct search_value<T[N]> {
    using val_type = T[N];    
    struct data_type {
        val_type val;
        data_type(const val_type & in){
            memcpy_pod(val, in);
        }
    };
    using vector = std::vector<data_type>;
    vector values;
    search_value(std::initializer_list<val_type> in): values(in.begin(), in.end()) {}
    search_value(search_value && src) : values(std::move(src.values)) {}
    search_value(search_value const & src) : values(src.values) {}
};

template<condition _c, class T> // T = col::
struct SEARCH {
    using value_type = search_value<typename T::val_type>;
    static const condition cond = _c;
    using col = T;
    value_type value;
    SEARCH(std::initializer_list<typename T::val_type> in): value(in) {}
};

template<class T> struct select_search_value {
    using type = NullType;//identity<T>;
};

template<condition _c, class T>
struct select_search_value<SEARCH<_c, T>> {
    using type = typename SEARCH<_c, T>::value_type;
};

template<class T>
using search_value_t = typename select_search_value<T>::type;

template<class T> using WHERE       = SEARCH<condition::WHERE, T>;
template<class T> using IN          = SEARCH<condition::IN, T>;
template<class T> using NOT         = SEARCH<condition::NOT, T>;
template<class T> using LESS        = SEARCH<condition::LESS, T>;
template<class T> using GREATER     = SEARCH<condition::GREATER, T>;
template<class T> using LESS_EQ     = SEARCH<condition::LESS_EQ, T>;
template<class T> using GREATER_EQ  = SEARCH<condition::GREATER_EQ, T>;
template<class T> using BETWEEN     = SEARCH<condition::BETWEEN, T>;

template<bool b>
struct USE_INDEX_IF {
    static const bool value = b;
};

using USE_INDEX = USE_INDEX_IF<true>;
using IGNORE_INDEX = USE_INDEX_IF<false>;

//-------------------------------------------------------------------

enum class operator_ { OR, AND };

template <operator_ T, class U = NullType>
struct operator_list {
    static const operator_ Head = T;
    typedef U Tail;
};

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
    operator_processor<TList>::apply(trace_operator(&count));
}

struct trace_SEARCH {
    size_t & count;
    explicit trace_SEARCH(size_t * p) : count(*p){}

    template<condition _c, class T> // T = col::
    void operator()(identity<SEARCH<_c, T>>) {
        const char * const col_name = typeid(T).name();
        const char * const val_name = typeid(typename T::val_type).name();
        SDL_TRACE(++count, ":", condition_name<_c>(), "<", col_name, ">", " (", val_name, ")");
    }
    template<class T, sortorder ord> 
    void operator()(identity<ORDER_BY<T, ord>>) {
        SDL_TRACE(++count, ":ORDER_BY<", typeid(T).name(), "> ", to_string::type_name(ord));
    }
    template<class T>
    void operator()(identity<T>) {
        SDL_TRACE(++count, ":", typeid(T).name());
    }
};

template<class TList> 
inline void trace_search_list() {
    size_t count = 0;
    meta::processor<TList>::apply(where_::trace_SEARCH(&count));
}

} //where_

namespace select_ { //FIXME: prototype

using operator_ = where_::operator_;

template<class record_range, class TList, class OList>
struct sub_expr
{    
    using type_list = TList;
    using oper_list = OList;
    using this_expr = sub_expr<record_range, TList, OList>;
private:
    template<class T, operator_ OP>
    using ret_expr = sub_expr<
            record_range, 
            Typelist<T, type_list>,
            where_::operator_list<OP, oper_list>
    >;
    template<class T> struct sub_expr_value;
    template<class T, sortorder ord>
    struct sub_expr_value<where_::ORDER_BY<T, ord>> {
        NullType move() {
            return{};
        }
    };        
    template<bool b>
    struct sub_expr_value<where_::USE_INDEX_IF<b>> {
        NullType move() {
            return{};
        }
    };
    template<where_::condition _c, class T>
    struct sub_expr_value<where_::SEARCH<_c, T>> {
    private:
        using param_t = where_::SEARCH<_c, T>;
        using value_t = typename param_t::value_type;
    public:
        value_t value;
        sub_expr_value(param_t const & s): value(s.value) {
            A_STATIC_ASSERT_TYPE(typename value_t::val_type, typename T::val_type);
            A_STATIC_ASSERT_NOT_TYPE(typename value_t::vector, NullType);
            SDL_ASSERT(!value.values.empty());
        }
        value_t && move() {
            return std::move(value);
        }
    };
    template<class T> // T = sub_expr
    struct sub_expr_value {
    private:
        using value_t = typename T::value_type;
    public:
        value_t value;
        sub_expr_value(T const & s): value(s.value) {
            A_STATIC_ASSERT_NOT_TYPE(typename T::this_expr, NullType);                
        }
        value_t && move() {
            return std::move(value);
        }
    };
public:
    using value_type = sub_expr_value<typename TList::Head>;
    value_type value;
    //using tail_value = where_::search_value_t<typename TList::Tail::Head>;
    //tail_value tail;
public:
    template<where_::condition _c, class T>
    sub_expr(where_::SEARCH<_c, T> const & s)
        : value(s)
    {
    }        
    template<where_::condition _c, class T, class tail_value>
    sub_expr(where_::SEARCH<_c, T> const & s, tail_value &&)
        : value(s)
    {
    }        
    template<class T, sortorder ord, class tail_value>
    sub_expr(where_::ORDER_BY<T, ord> const &, tail_value &&)
        : value{}
    {
    }        
    template<bool b, class tail_value>
    sub_expr(where_::USE_INDEX_IF<b> const &, tail_value &&)
        : value{}
    {
    }        
    template<class T1, class T2, class T3, class tail_value>
    sub_expr(sub_expr<T1, T2, T3> const & s, tail_value &&)
        : value(s)
    {
    }
public:
    template<class T> // T = where_::SEARCH or sub_expr
    ret_expr<T, operator_::OR> operator | (T && s) { // Note. T && is "universal reference"
        return { std::forward<T>(s), value.move() };
    }
    template<class T> // T = where_::SEARCH or sub_expr
    ret_expr<T, operator_::AND> operator && (T && s) { // Note. T && is "universal reference"
        return { std::forward<T>(s), value.move() };
    }
    record_range VALUES() {
        using T1 = typename TL::Reverse<type_list>::Result;
        using T2 = typename where_::reverse<oper_list>::Result;
        if (1) {
            SDL_TRACE("\nVALUES:");
            where_::trace_search_list<T1>();
            where_::trace_operator_list<T2>();
        }
        return {};
    }
    operator record_range() { 
        return VALUES();
    }
};

template<class record_range>
class select_expr : noncopyable
{   
    template<class T, operator_ OP>
    using ret_expr = sub_expr<
        record_range, 
        Typelist<T, NullType>,
        where_::operator_list<OP>
    >;
public:
    template<class T> // T = where_::SEARCH or sub_expr
    ret_expr<T, operator_::OR> operator | (T && s) { // Note. T && is "universal reference"
        return { std::forward<T>(s) };
    }
    template<class T> // T = where_::SEARCH or sub_expr
    ret_expr<T, operator_::AND> operator && (T && s) { // Note. T && is "universal reference"
        return { std::forward<T>(s) };
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
private:
    using select_expr = select_::select_expr<record_range>;
public:
    select_expr SELECT;
};

} // make
} // db
} // sdl

#include "maketable.hpp"

#endif // __SDL_SYSTEM_MAKETABLE_H__
