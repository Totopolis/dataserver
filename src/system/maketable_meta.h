// maketable_meta.h
//
#pragma once
#ifndef __SDL_SYSTEM_MAKETABLE_META_H__
#define __SDL_SYSTEM_MAKETABLE_META_H__

#include "common/type_seq.h"
#include "common/static_type.h"
#include "page_info.h"

namespace sdl { namespace db { namespace make { namespace meta {

template<bool _PK, size_t pos, sortorder ord>
struct key {
    enum { PK = _PK }; // is_primary_key
    enum { key_pos = pos };
    static const sortorder order = ord;
};
using key_true = key<true, 0, sortorder::ASC>;
using key_false = key<false, 0, sortorder::NONE>;

template<scalartype::type, int> struct value_type; 
template<> struct value_type<scalartype::t_int, 4> {
    using type = int;
    enum { fixed = 1 };
};  
template<> struct value_type<scalartype::t_bigint, 8> {
    using type = uint64;
    enum { fixed = 1 };
};
template<> struct value_type<scalartype::t_smallint, 2> {
    using type = uint16;
    enum { fixed = 1 };
};
template<> struct value_type<scalartype::t_tinyint, 1> {
    using type = uint8;
    enum { fixed = 1 };
}; 
template<> struct value_type<scalartype::t_float, 8> { 
    using type = double;
    enum { fixed = 1 };
};
template<> struct value_type<scalartype::t_real, 4> { 
    using type = float;
    enum { fixed = 1 };
};
template<> struct value_type<scalartype::t_smalldatetime, 4> { 
    using type = smalldatetime_t;
    enum { fixed = 1 };
};
template<> struct value_type<scalartype::t_uniqueidentifier, 16> { 
    using type = guid_t;
    enum { fixed = 1 };
};
template<> struct value_type<scalartype::t_numeric, 9> { 
    using type = numeric9;
    enum { fixed = 1 };
};
template<> struct value_type<scalartype::t_smallmoney, 4> { 
    using type = uint32; //FIXME: not implemented
    enum { fixed = 1 };
};
template<int len> 
struct value_type<scalartype::t_char, len> {
    using type = char[len];
    enum { fixed = 1 };
};
template<int len> 
struct value_type<scalartype::t_nchar, len> {
    using type = nchar_t[(len % 2) ? 0 : (len/2)];
    enum { fixed = 1 };
};
template<int len> 
struct value_type<scalartype::t_varchar, len> {
    using type = var_mem;
    enum { fixed = 0 };
};
template<int len> 
struct value_type<scalartype::t_text, len> {
    using type = var_mem;
    enum { fixed = 0 };
};
template<int len> 
struct value_type<scalartype::t_ntext, len> {
    using type = var_mem;
    enum { fixed = 0 };
};
template<int len> 
struct value_type<scalartype::t_nvarchar, len> {
    using type = var_mem;
    enum { fixed = 0 };
};
template<> struct value_type<scalartype::t_varbinary, -1> {
    using type = var_mem;
    enum { fixed = 0 };
};
/*template<> struct value_type<scalartype::t_geometry, -1> {
    using type = var_mem;
    enum { fixed = 0 };
};*/
template<> struct value_type<scalartype::t_geography, -1> {
    using type = var_mem;
    enum { fixed = 0 };
};

template <bool v> struct is_fixed { enum { value = v }; };
template <bool v> struct is_array { enum { value = v }; };

template <class TList> struct IsFixed;
template <> struct IsFixed<NullType> {
    enum { value = 1 };
};
template <class T, class U>
struct IsFixed< Typelist<T, U> > {
    enum { value = T::fixed && IsFixed<U>::value };
};

template<size_t _place, size_t off, scalartype::type _type, int len, typename base_key = key_false>
struct col : base_key {
private:
    using traits = value_type<_type, len>;
    using T = typename traits::type;
    col() = delete;
public:
    enum { fixed = traits::fixed };
    enum { offset = off };
    enum { length = len };
    enum { place = _place };
    enum { is_array = std::is_array<T>::value };
    using val_type = T;
    using ret_type = typename std::conditional<is_array, T const &, T>::type;
    static const scalartype::type type = _type;
    static void test() {
        static_assert(!fixed || (length > 0), "col::length");
        static_assert(!fixed || (length == sizeof(val_type)), "col::val_type");
    }
};

template<class T, size_t off = 0>
struct index_col {
    using col = T;
    using type = typename col::val_type;
    enum { offset = off };
};

//------------------------------------------------------------------------------

template<class T, class empty> struct _cluster_key {
    using type = typename T::key_type;
};
template<class empty> struct _cluster_key<void, empty> {
    using type = empty;
};
template<class T, class empty>
using cluster_key = typename _cluster_key<T, empty>::type;

//------------------------------------------------------------------------------

template<class T, class empty> struct _cluster_type_list {
    using type = typename T::type_list;
};
template<class empty> struct _cluster_type_list<void, empty> {
    using type = empty;
};
template<class T, class empty>
using cluster_type_list = typename _cluster_type_list<T, empty>::type;

//------------------------------------------------------------------------------

template<class T> struct cluster_index_size {
    enum { value = T::index_size };
};
template<> struct cluster_index_size<void> {
    enum { value = 0 };
};

//------------------------------------------------------------------------------

template <class TList, class col_type> 
struct cluster_col_find;

template <class col_type> struct cluster_col_find<NullType, col_type> {
    enum { value = -1 };
};

template <class Head, class Tail>
struct cluster_col_find<Typelist<Head, Tail>, typename Head::col> {
    enum { value = 0 };
};

template <class Head, class Tail, class col_type>
struct cluster_col_find<Typelist<Head, Tail>, col_type> {
private:
    enum { temp = cluster_col_find<Tail, col_type>::value };
public:
    enum { value = (temp == -1 ? -1 : 1 + temp) };
};

//------------------------------------------------------------------------------

template<class T>
struct test_clustered_ {
    static bool test() {
        using key_type = cluster_key<T, void>;
        using type_list = typename T::type_list;
        static_assert(std::is_pod<key_type>::value, "");
        enum { index_size = TL::Length<type_list>::value };       
        using last = typename TL::TypeAt<type_list, index_size - 1>::Result;
        static_assert(sizeof(key_type()._0), "");
        static_assert(sizeof(key_type) == (last::offset + sizeof(typename last::type)), "");
        return true;
    }
};

template<> struct test_clustered_<void> {
    static bool test() { return true; }
};

template<class T> inline
bool test_clustered() {
    return test_clustered_<T>::test();
}

//-----------------------------------------------------------

template<class TList> struct processor;
template<> struct processor<NullType> {
    template<class fun_type>
    static void apply(fun_type){}
};
template <class T, class U>
struct processor<Typelist<T, U>> {
    template<class fun_type>
    static void apply(fun_type fun){
        fun(identity<T>());
        processor<U>::apply(fun);
    }
};

struct trace_type {
    size_t & count;
    explicit trace_type(size_t * p) : count(*p){}
    template<class T> 
    void operator()(identity<T>) {
        SDL_TRACE(++count, ":", typeid(T).name());
    }
};

template<class TList> 
inline void trace_typelist() {
    size_t count = 0;
    processor<TList>::apply(trace_type(&count));
}

//-----------------------------------------------------------

template<class T, sortorder ord>
struct is_less_t;

template<class T> 
struct is_less_t<T, sortorder::ASC> {    
    template<class arg_type>
    static bool less(arg_type const & x, arg_type const & y) {
        A_STATIC_ASSERT_TYPE(arg_type, typename T::type);
        static_assert(!std::is_array<arg_type>::value, "");
        return x < y;
    }
    template<size_t N>
    static bool less(char const (&x)[N], char const (&y)[N]) {
        A_STATIC_ASSERT_TYPE(char[N], typename T::type);
        return ::memcmp(x, y, sizeof(x)) < 0;
    }
    template<size_t N>
    static bool less(nchar_t const (&x)[N], nchar_t const (&y)[N]) {
        A_STATIC_ASSERT_TYPE(nchar_t[N], typename T::type);
        return nchar_less(x, y);
    }
};

template<class T> 
struct is_less_t<T, sortorder::DESC> {
    template<class arg_type>
    static bool less(arg_type const & x, arg_type const & y) {
        A_STATIC_ASSERT_TYPE(arg_type, typename T::type);
        return y < x;
    }
    template<size_t N>
    static bool less(char const (&x)[N], char const (&y)[N]) {
        A_STATIC_ASSERT_TYPE(char[N], typename T::type);
        return ::memcmp(y, x, sizeof(x)) < 0;
    }
    template<size_t N>
    static bool less(nchar_t const (&x)[N], nchar_t const (&y)[N]) {
        A_STATIC_ASSERT_TYPE(nchar_t[N], typename T::type);
        return nchar_less(y, x);

    }
};

template<class T>  // T = meta::index_col
using is_less = is_less_t<T, T::col::order>;

//-----------------------------------------------------------

template<class T>
inline void copy(T & dest, T const & src) {
    dest = src;
}

template<class T, size_t N>
inline void copy(T (&dest)[N], T const (&src)[N]) {
    A_STATIC_ASSERT_IS_POD(T[N]);
    memcpy(dest, src, sizeof(dest));
}

//-----------------------------------------------------------

template<class type_list>
class key_to_string : is_static {

    enum { index_size = TL::Length<type_list>::value };

    template<class key_type>
    struct fun_type {
        std::string & result;
        key_type const & data;
        fun_type(std::string & s, key_type const & d): result(s), data(d){}

        template<class T> // T = identity<meta::index_col>
        void operator()(identity<T>) {
            enum { get_i = TL::IndexOf<type_list, T>::value };
            if (get_i) result += ",";
            result += to_string::type(data.get(Int2Type<get_i>()));
        }
    };

public:
    template<class key_type>
    static std::string to_str(key_type const & m) {
        std::string result;
        if (index_size > 1) result += "(";
        processor<type_list>::apply(fun_type<key_type>(result, m));
        if (index_size > 1) result += ")";
        return result;
    }
};

//-----------------------------------------------------------

} // meta
} // make
} // db
} // sdl

#endif // __SDL_SYSTEM_MAKETABLE_META_H__
