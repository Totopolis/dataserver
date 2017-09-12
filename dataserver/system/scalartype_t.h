// scalartype_t.h
//
#pragma once
#ifndef __SDL_SYSTEM_SCALARTYPE_T_H__
#define __SDL_SYSTEM_SCALARTYPE_T_H__

#include "dataserver/system/page_type.h"
#include "dataserver/common/static_type.h"

namespace sdl { namespace db {

template<scalartype::type> struct scalartype_to_key;
template<typename> struct key_to_scalartype;

#define define_scalartype_to_key(src, dest) \
    template<> struct scalartype_to_key<src> { \
        static constexpr scalartype::type value = src; \
        using type = dest; \
    }; \
    template<> struct key_to_scalartype<dest> { \
        static constexpr scalartype::type value = src; \
    };

define_scalartype_to_key(scalartype::t_int,                 int32)
define_scalartype_to_key(scalartype::t_bigint,              int64)
define_scalartype_to_key(scalartype::t_uniqueidentifier,    guid_t)
define_scalartype_to_key(scalartype::t_float,               double)
define_scalartype_to_key(scalartype::t_real,                float)
define_scalartype_to_key(scalartype::t_smallint,            int16)
define_scalartype_to_key(scalartype::t_tinyint,             int8)
define_scalartype_to_key(scalartype::t_smalldatetime,       smalldatetime_t)
define_scalartype_to_key(scalartype::t_datetime,            datetime_t) //FIXME: Defines a date that is combined with a time of day with fractional seconds that is based on a 24-hour clock
define_scalartype_to_key(scalartype::t_datetime2,           datetime2_t)//FIXME: precision must be 7 digits
define_scalartype_to_key(scalartype::t_smallmoney,          uint32)     //FIXME: not implemented
define_scalartype_to_key(scalartype::t_money,               uint64)     //FIXME: not implemented
define_scalartype_to_key(scalartype::t_bit,                 uint8)      //FIXME: If there are 8 or less bit columns in a table, the columns are stored as 1 byte. If there are from 9 up to 16 bit columns, the columns are stored as 2 bytes, and so on
//define_scalartype_to_key(scalartype::t_numeric,           numeric9)   //FIXME: variable size
//define_scalartype_to_key(scalartype::t_decimal,           decimal5)   //FIXME: variable size

template<scalartype::type v> 
using scalartype_t = typename scalartype_to_key<v>::type;

struct scalartype_to_key_list {
    typedef TL::Seq<
        scalartype_to_key<scalartype::t_int>,
        scalartype_to_key<scalartype::t_bigint>,
        scalartype_to_key<scalartype::t_uniqueidentifier>,
        scalartype_to_key<scalartype::t_float>,
        scalartype_to_key<scalartype::t_real>,
        scalartype_to_key<scalartype::t_smallint>,
        scalartype_to_key<scalartype::t_tinyint>,
        scalartype_to_key<scalartype::t_smalldatetime>,
        scalartype_to_key<scalartype::t_datetime>,
        scalartype_to_key<scalartype::t_smallmoney>,
        scalartype_to_key<scalartype::t_money>,
        scalartype_to_key<scalartype::t_bit>
        //scalartype_to_key<scalartype::t_numeric>,
        //scalartype_to_key<scalartype::t_decimal>
    >::Type type_list;
};

template<class fun_type> 
struct case_scalartype_ret_type {
private:
    template<typename T> static auto check(void *) -> typename T::ret_type;
    template<typename T> static void check(...);
public:
    using type = decltype(check<fun_type>(nullptr));
};

template <class type_list>
struct for_scalartype_to_key;

template <> struct for_scalartype_to_key<NullType> {
    template<class fun_type> static
    typename case_scalartype_ret_type<fun_type>::type
    find(scalartype::type, fun_type &&){
        SDL_ASSERT(0);
        return {};
    }
};

template <class T, class U> // T = scalartype_to_key
struct for_scalartype_to_key<Typelist<T, U>> {
    template<class fun_type> static
    typename case_scalartype_ret_type<fun_type>::type
    find(scalartype::type const v, fun_type && fun){
        if (v == T::value) {
            return fun(T());
        }
        return for_scalartype_to_key<U>::find(v, std::forward<fun_type>(fun));
    }
};

using case_scalartype_to_key = for_scalartype_to_key<scalartype_to_key_list::type_list>;

} // db
} // sdl

#endif // __SDL_SYSTEM_SCALARTYPE_T_H__
