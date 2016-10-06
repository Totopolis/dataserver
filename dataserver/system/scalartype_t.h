// scalartype_t.h
//
#pragma once
#ifndef __SDL_SYSTEM_SCALARTYPE_T_H__
#define __SDL_SYSTEM_SCALARTYPE_T_H__

#include "page_type.h"

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
define_scalartype_to_key(scalartype::t_numeric,             numeric9)
define_scalartype_to_key(scalartype::t_smalldatetime,       smalldatetime_t)
define_scalartype_to_key(scalartype::t_datetime,            datetime_t) //FIXME: Defines a date that is combined with a time of day with fractional seconds that is based on a 24-hour clock
define_scalartype_to_key(scalartype::t_smallmoney,          uint32)     //FIXME: not implemented
define_scalartype_to_key(scalartype::t_bit,                 uint8)      //FIXME: If there are 8 or less bit columns in a table, the columns are stored as 1 byte. If there are from 9 up to 16 bit columns, the columns are stored as 2 bytes, and so on
define_scalartype_to_key(scalartype::t_decimal,             decimal5)

template<scalartype::type v> 
using scalartype_t = typename scalartype_to_key<v>::type;

template<class fun_type> 
struct case_scalartype_ret_type {
private:
    template<typename T> static auto check(void *) -> typename T::ret_type;
    template<typename T> static void check(...);
public:
    using type = decltype(check<fun_type>(nullptr));
};

template<typename ret_type, class fun_type>
ret_type case_scalartype_to_key_t(scalartype::type const v, fun_type && fun) {
    switch (v) {
    case scalartype::t_int              : return fun(scalartype_to_key<scalartype::t_int>());
    case scalartype::t_bigint           : return fun(scalartype_to_key<scalartype::t_bigint>());
    case scalartype::t_uniqueidentifier : return fun(scalartype_to_key<scalartype::t_uniqueidentifier>());
    case scalartype::t_float            : return fun(scalartype_to_key<scalartype::t_float>());
    case scalartype::t_real             : return fun(scalartype_to_key<scalartype::t_real>());
    case scalartype::t_smallint         : return fun(scalartype_to_key<scalartype::t_smallint>());
    case scalartype::t_tinyint          : return fun(scalartype_to_key<scalartype::t_tinyint>());
    case scalartype::t_numeric          : return fun(scalartype_to_key<scalartype::t_numeric>());
    case scalartype::t_smalldatetime    : return fun(scalartype_to_key<scalartype::t_smalldatetime>());
    case scalartype::t_datetime         : return fun(scalartype_to_key<scalartype::t_datetime>());
    case scalartype::t_smallmoney       : return fun(scalartype_to_key<scalartype::t_smallmoney>());
    case scalartype::t_bit              : return fun(scalartype_to_key<scalartype::t_bit>());
    case scalartype::t_decimal          : return fun(scalartype_to_key<scalartype::t_decimal>());
    default:
        SDL_ASSERT(0);
        return ret_type();
    }
}

template<class fun_type> inline 
typename case_scalartype_ret_type<fun_type>::type
case_scalartype_to_key(scalartype::type const value, fun_type && fun) {
    using ret_type = typename case_scalartype_ret_type<fun_type>::type;
    return case_scalartype_to_key_t<ret_type>(value, std::forward<fun_type>(fun));
}

} // db
} // sdl

#endif // __SDL_SYSTEM_SCALARTYPE_T_H__
