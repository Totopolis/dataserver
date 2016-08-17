// scalartype_t.h
//
#pragma once
#ifndef __SDL_SYSTEM_SCALARTYPE_T_H__
#define __SDL_SYSTEM_SCALARTYPE_T_H__

#include "page_type.h"

namespace sdl { namespace db { namespace impl {

template<scalartype::type> struct scalartype_to_key;
template<> struct scalartype_to_key<scalartype::t_int>               { using type = int32; };
template<> struct scalartype_to_key<scalartype::t_bigint>            { using type = int64; };
template<> struct scalartype_to_key<scalartype::t_uniqueidentifier>  { using type = guid_t; };
template<> struct scalartype_to_key<scalartype::t_float>             { using type = double; };
template<> struct scalartype_to_key<scalartype::t_real>              { using type = float; };
template<> struct scalartype_to_key<scalartype::t_smallint>          { using type = int16; };
template<> struct scalartype_to_key<scalartype::t_tinyint>           { using type = int8; };

} // impl

template<class T> struct key_to_scalartype;
template<> struct key_to_scalartype<int32>  { static const scalartype::type value = scalartype::t_int; };
template<> struct key_to_scalartype<int64>  { static const scalartype::type value = scalartype::t_bigint; };
template<> struct key_to_scalartype<guid_t> { static const scalartype::type value = scalartype::t_uniqueidentifier; };
template<> struct key_to_scalartype<double> { static const scalartype::type value = scalartype::t_float; };
template<> struct key_to_scalartype<float>  { static const scalartype::type value = scalartype::t_real; };
template<> struct key_to_scalartype<int16>  { static const scalartype::type value = scalartype::t_smallint; };
template<> struct key_to_scalartype<int8>   { static const scalartype::type value = scalartype::t_tinyint; };

template<scalartype::type v> using scalartype_t = typename impl::scalartype_to_key<v>::type;

} // db
} // sdl

#endif // __SDL_SYSTEM_SCALARTYPE_T_H__