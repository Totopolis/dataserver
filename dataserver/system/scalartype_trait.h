// scalartype_trait.h
//
#pragma once
#ifndef __SDL_SYSTEM_SCALAR_TYPE_TRAIT_H__
#define __SDL_SYSTEM_SCALAR_TYPE_TRAIT_H__

#include "page_type.h"

namespace sdl { namespace db {

template<scalartype::type T>
struct scalartype_is_text {
    static constexpr bool value = false;
};
template<> struct scalartype_is_text<scalartype::t_text>    { static constexpr bool value = true; };
template<> struct scalartype_is_text<scalartype::t_char>    { static constexpr bool value = true; };
template<> struct scalartype_is_text<scalartype::t_varchar> { static constexpr bool value = true; };

template<scalartype::type T>
struct scalartype_is_ntext {
    static constexpr bool value = false;
};
template<> struct scalartype_is_ntext<scalartype::t_ntext>    { static constexpr bool value = true; };
template<> struct scalartype_is_ntext<scalartype::t_nchar>    { static constexpr bool value = true; };
template<> struct scalartype_is_ntext<scalartype::t_nvarchar> { static constexpr bool value = true; };

template<scalartype::type T>
struct scalartype_trait {
    static constexpr scalartype::type value = T;
    static constexpr bool is_text = scalartype_is_text<T>::value;
    static constexpr bool is_ntext = scalartype_is_ntext<T>::value;
};

} // db
} // sdl

#endif // __SDL_SYSTEM_SCALAR_TYPE_TRAIT_H__