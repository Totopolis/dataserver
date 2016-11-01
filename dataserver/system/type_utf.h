// type_utf.h
//
#pragma once
#ifndef __SDL_SYSTEM_TYPE_UTF_H__
#define __SDL_SYSTEM_TYPE_UTF_H__

#include "page_info.h"
#include "utils/conv.h"

namespace sdl { namespace db { namespace info {

template<bool b> struct is_text_const : bool_constant<b>{};
template<bool b> struct is_ntext_const : bool_constant<b>{};
template<bool b> struct is_trim_const : bool_constant<b>{};

template<scalartype::type, typename... Ts>
struct type_col_utf8;

//----------------------------------------------------------------------------------

template<scalartype::type T>
struct type_col_utf8<T, is_text_const<false>, is_ntext_const<false>> {
    template<class value_type>
    static std::string type_col(value_type && value, is_trim_const<false>) {
        std::string s = to_string::type(std::forward<value_type>(value));
        SDL_ASSERT(conv::is_utf8(s));
        return s;
    }
    template<class value_type>
    static std::string type_col(value_type && value, is_trim_const<true>) {
        std::string s = to_string::trim(to_string::type(std::forward<value_type>(value)));
        SDL_ASSERT(conv::is_utf8(s));
        return s;
    }
};

//----------------------------------------------------------------------------------

template<scalartype::type T>
struct type_col_utf8<T, is_text_const<true>, is_ntext_const<false>> {
    template<class value_type>
    static std::string type_col(value_type && value, is_trim_const<false>) {
        return conv::cp1251_to_utf8(to_string::type(std::forward<value_type>(value)));
    }
    template<class value_type>
    static std::string type_col(value_type && value, is_trim_const<true>) {
        return conv::cp1251_to_utf8(to_string::trim(to_string::type(std::forward<value_type>(value))));
    }
};

template<scalartype::type T>
struct type_col_utf8<T, is_text_const<false>, is_ntext_const<true>> {
    static std::string type_col(var_mem const & value, is_trim_const<false>) {
        return conv::nchar_to_utf8(value.cdata());
    }
    static std::string type_col(var_mem const & value, is_trim_const<true>) {
        return to_string::trim(conv::nchar_to_utf8(value.cdata()));
    }
};

//----------------------------------------------------------------------------------

template<scalartype::type T>
using type_col_utf8_t = type_col_utf8<T,
    is_text_const<scalartype::is_text(T)>, 
    is_ntext_const<scalartype::is_ntext(T)>>;

template<scalartype::type T>
struct type_col_wide_t {
    template<class value_type, bool is_trim>
    static std::wstring type_col(value_type && value, is_trim_const<is_trim>) {
        return conv::utf8_to_wide(
            type_col_utf8_t<T>::type_col(std::forward<value_type>(value), 
            is_trim_const<is_trim>()));
    }
};

//----------------------------------------------------------------------------------

} // info
} // db
} // sdl

#endif // __SDL_SYSTEM_TYPE_UTF_H__