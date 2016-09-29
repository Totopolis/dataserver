// maketable_meta.h
//
#pragma once
#ifndef __SDL_SYSTEM_MAKETABLE_META_H__
#define __SDL_SYSTEM_MAKETABLE_META_H__

#include "common/type_seq.h"
#include "common/static_type.h"
#include "system/page_info.h"
#include "system/scalartype_t.h"
#include "spatial/spatial_tree_t.h"

namespace sdl { namespace db { namespace make { namespace meta {

enum class key_t {
    null, // no_key
    primary_key,
    spatial_key
};
template<key_t KEY, size_t pos, sortorder ord>
struct base_key {
    static constexpr key_t key = KEY;
    enum { PK = (key == key_t::primary_key) };
    enum { spatial_key = (key == key_t::spatial_key) };
    enum { key_pos = pos };
    static constexpr sortorder order = ord;
};

template<bool PK, size_t pos, sortorder ord>
using key = base_key<PK ? key_t::primary_key : key_t::null, pos, ord>;
using key_true = key<true, 0, sortorder::ASC>;
using key_false = key<false, 0, sortorder::NONE>;
using spatial_key = base_key<key_t::spatial_key, 0, sortorder::ASC>;

template<scalartype::type, int> struct value_type;
template<> struct value_type<scalartype::t_int, 4> {
    using type = int32; //FIXME: compare with scalartype_t
    enum { fixed = 1 };
};  
template<> struct value_type<scalartype::t_bigint, 8> {
    using type = int64;
    enum { fixed = 1 };
};
template<> struct value_type<scalartype::t_smallint, 2> {
    using type = int16;
    enum { fixed = 1 };
};
template<> struct value_type<scalartype::t_tinyint, 1> {
    using type = int8;
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
template<> struct value_type<scalartype::t_bit, 1> { 
    using type = uint8; //FIXME: If there are 8 or less bit columns in a table, the columns are stored as 1 byte. If there are from 9 up to 16 bit columns, the columns are stored as 2 bytes, and so on
    enum { fixed = 1 };
};
template<> struct value_type<scalartype::t_datetime, 8> { 
    using type = uint64; //FIXME: Defines a date that is combined with a time of day with fractional seconds that is based on a 24-hour clock
    enum { fixed = 1 };
};
template<> struct value_type<scalartype::t_decimal, 5> {
    using type = decimal5;
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
template<> struct value_type<scalartype::t_geometry, -1> {
    using type = var_mem;
    enum { fixed = 0 };
};
template<> struct value_type<scalartype::t_geography, -1> {
    using type = geo_mem;
    enum { fixed = 0 };
};
template <bool v> struct is_fixed { enum { value = v }; };
template <bool v> struct is_array { enum { value = v }; };
template <bool v> struct is_key { enum { value = v }; };

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
    //enum { is_unique = bool };
    using val_type = T;
    using ret_type = typename std::conditional<is_array, T const &, T>::type;
    static constexpr scalartype::type type = _type;
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

template<schobj_id::type _id, index_id::type _indid, idxtype::type _type>
struct idxstat {
    static constexpr schobj_id::type schobj = _id;  // the object_id of the table or view that this index belongs to
    static constexpr index_id::type indid = _indid; // the index_id (1 for the clustered index, larger numbers for non-clustered indexes)
    static constexpr idxtype::type type = _type;
    enum { is_clustered = (type == idxtype::clustered) };
    enum { is_spatial = (type == idxtype::spatial) };
};

//------------------------------------------------------------------------------

template<class T> // T = table::clustered
struct clustered_traits {
    using key_type = typename T::key_type;
    using type_list = typename T::type_list;
    using T0_col = typename T::T0::col;
    using T0_type = typename T::T0::type;
    using spatial_tree_T0 = spatial_tree_t<T0_type>;
    using spatial_page_row = typename spatial_tree_T0::spatial_page_row;
    enum { index_size = T::index_size };
};
template<> struct clustered_traits<void> {
    using key_type = NullType;
    using type_list = NullType;
    using T0_col = NullType;
    using T0_type = NullType;
    using spatial_tree_T0 = NullType;
    using spatial_page_row = NullType;
    enum { index_size = 0 };
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
        using key_type = typename clustered_traits<T>::key_type;
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

template<class TList> struct processor_if;
template<> struct processor_if<NullType> {
    template<class fun_type>
    static void apply(fun_type &&){}
};
template <class T, class U>
struct processor_if<Typelist<T, U>> {
    template<class fun_type>
    static void apply(fun_type && fun){
        if (make_break_or_continue(fun(identity<T>())) == bc::continue_) {
            processor_if<U>::apply(fun);
        }
    }
};

struct trace_type {
    size_t & count;
    explicit trace_type(size_t * p) : count(*p){}
    template<class T> 
    bool operator()(identity<T>) {
        SDL_TRACE(count++, ":", short_name(typeid(T).name()));
        return true;
    }
    static std::string short_name(const char *);
};

template<class T>
inline std::string short_name() {
    return trace_type::short_name(typeid(T).name());
}

template<class TList> 
inline void trace_typelist() {
    size_t count = 0;
    processor_if<TList>::apply(trace_type(&count));
}

//-----------------------------------------------------------

template<class T, sortorder ord>    //FIXME: locale hint = (none, en, rus); trim spaces; case insensitive
struct is_less_t;

template<class T> 
struct is_less_t<T, sortorder::ASC> {    
    template<class arg_type>
    static bool less(arg_type const & x, arg_type const & y) {
        A_STATIC_ASSERT_TYPE(arg_type, typename T::type);
        static_assert(!std::is_array<arg_type>::value, "");
        return x < y; //FIXME: if float type compare with tolerance ?
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
        return y < x; //FIXME: if float type compare with tolerance ?
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

template<class col>
using key_less = is_less_t<identity<typename col::val_type>, col::order>;

template<class col, sortorder ord>
using col_less = is_less_t<identity<typename col::val_type>, ord>;

//-----------------------------------------------------------

template<class T, bool is_array> struct is_equal_t;

template<> struct is_equal_t<float, false> {
    static bool equal(float x, float y) {
        return fequal(x, y);
    }
};

template<> struct is_equal_t<double, false> {
    static bool equal(double x, double y) {
        return fequal(x, y);
    }
};

template<class T>
struct is_equal_t<T, false> {
    static bool equal(T const & x, T const & y) {
        static_assert(!std::is_array<T>::value, "");
        static_assert(!std::is_floating_point<T>::value, "");
        static_assert(sizeof(T) <= 8, "");
        return x == y;
    }
};

template<class T>
struct is_equal_t<T, true> {
    static bool equal(T const & x, T const & y) {
        using _Elem = typename std::remove_extent<T>::type;
        static_assert(std::is_array<T>::value, "");
        static_assert(!std::is_floating_point<T>::value, "");
        static_assert(std::is_same<char, _Elem>::value || std::is_same<nchar_t, _Elem>::value, "");
        return memcmp_pod(x, y) == 0;
    }
};

template<class T> using is_equal = is_equal_t<typename T::val_type, T::is_array>;

//-----------------------------------------------------------

template<class T> struct copy_t;
template<class T>
struct copy_t {
    static void apply(T & dest, T const & src) {
        dest = src;
    }
};

template<class T, size_t N>
struct copy_t<T[N]> {
    static void apply(T (&dest)[N], T const (&src)[N]) {
        A_STATIC_ASSERT_IS_POD(T[N]);
        memcpy(dest, src, sizeof(dest));
    }
};

template<class T>
inline void copy(T & dest, T const & src) {
    copy_t<T>::apply(dest, src);
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
        bool operator()(identity<T>) {
            enum { get_i = TL::IndexOf<type_list, T>::value };
            if (get_i) result += ",";
            result += to_string::type(data.get(Int2Type<get_i>()));
            return true;
        }
    };

public:
    template<class key_type>
    static std::string to_str(key_type const & m) {
        std::string result;
        if (index_size > 1) result += "(";
        processor_if<type_list>::apply(fun_type<key_type>(result, m));
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
