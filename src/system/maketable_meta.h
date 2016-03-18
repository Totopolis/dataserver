// maketable_meta.h
//
#ifndef __SDL_SYSTEM_MAKETABLE_META_H__
#define __SDL_SYSTEM_MAKETABLE_META_H__

#pragma once

#include "common/type_seq.h"
#include "common/static_type.h"
#include "page_info.h"

namespace sdl { namespace db { namespace make { namespace meta {

template<bool PK, size_t id = 0, sortorder ord = sortorder::ASC>
struct key {
    enum { is_primary_key = PK };
    enum { subid = id };
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
    using type = char[9]; //FIXME: not implemented
    enum { fixed = 1 };
};
template<int len> 
struct value_type<scalartype::t_char, len> {
    using type = char[len];
    enum { fixed = 1 };
};
template<int len> 
struct value_type<scalartype::t_nchar, len> {
    using type = nchar_t[len];
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

template<size_t off, scalartype::type _type, int len = -1, typename base_key = key_false>
struct col : base_key {
private:
    using traits = value_type<_type, len>;
    using T = typename traits::type;
    col() = delete;
public:
    using val_type = T;
    using ret_type = typename std::conditional<std::is_array<T>::value, T const &, T>::type;
    enum { fixed = traits::fixed };
    enum { offset = off };
    enum { length = len };
    static const scalartype::type type = _type;
    static void test() {
        static_assert(!fixed || (length > 0), "col::length");
        static_assert(!fixed || (std::is_array<T>::value ? 
            (length == sizeof(val_type)/sizeof(typename std::remove_extent<T>::type)) :
            (length == sizeof(val_type))), "col::val_type");
    }
};

template<class T, size_t off = 0>
struct index_col {
    using col = T;
    using type = typename col::val_type;
    enum { offset = off };
};

template<class TYPE_LIST, size_t i>
using index_type = typename TL::TypeAt<TYPE_LIST, i>::Result::type; // = index_col::type

template<class T> struct cluster_key {
    using type = typename T::key_type;
};
template<> struct cluster_key<void> {
    using type = void;
};

template<class cluster_index>
struct _check_cluster_index {
    static bool check() {
        using cluster_key = typename cluster_key<cluster_index>::type;
        using type_list = typename cluster_index::type_list;
        static_assert(std::is_pod<cluster_key>::value, "");
        enum { index_size = TL::Length<type_list>::value };       
        using last = typename TL::TypeAt<type_list, index_size - 1>::Result;
        static_assert(sizeof(cluster_key), "");
        static_assert(sizeof(cluster_key) == (last::offset + sizeof(typename last::type)), "");
        return true;
    }
};

template<> struct _check_cluster_index<void> {
    static bool check() { return true; }
};

template<class T> inline
bool check_cluster_index() {
    return _check_cluster_index<T>::check();
}

#if 0
template<class TList, size_t> struct processor;
template<size_t index> struct processor<NullType, index> {
    template<class fun_type>
    static void apply(fun_type){}
};
template <class T, class U, size_t index>
struct processor<Typelist<T, U>, index> {
    template<class fun_type>
    static void apply(fun_type fun){
        //FIXME: simplify code for linux build
        //FIXME: fun(identity<T>(), Int2Type<index>());
        processor<U, index + 1>::apply(fun);
    }
};
#else
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
#endif

} // meta
} // make
} // db
} // sdl

#endif // __SDL_SYSTEM_MAKETABLE_META_H__
