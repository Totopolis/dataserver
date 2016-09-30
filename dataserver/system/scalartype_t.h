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
    template<> struct scalartype_to_key<src> { using type = dest; }; \
    template<> struct key_to_scalartype<dest> { \
        enum { enum_value = src }; \
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

} // db
} // sdl

#endif // __SDL_SYSTEM_SCALARTYPE_T_H__
