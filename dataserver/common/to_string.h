// to_string.h
//
#pragma once
#ifndef __SDL_COMMON_TO_STRING_H__
#define __SDL_COMMON_TO_STRING_H__

#include "dataserver/common/common.h"

namespace sdl {

class is_std_to_string_ {
    template<typename T> static auto check(T && x) -> decltype(std::to_string(x));
    template<typename T> static void check(...);
    template<typename T> static T && make();
public:
    template<typename T> static auto test() -> decltype(check<T>(make<T>()));
};

template<typename T> 
using is_std_to_string_t = decltype(is_std_to_string_::test<T>());

template<typename T> 
using is_std_to_string = bool_constant<!std::is_same<void,is_std_to_string_t<T>>::value>;

//------------------------------------------------------------

class str_util : is_static {
    template<typename T>
    static std::string impl_to_string(T && value, bool_constant<true>) {
        return std::to_string(value);
    }
    template<typename T>
    static std::string impl_to_string(T && value, bool_constant<false>) {
        std::stringstream ss; 
        ss << value;
        return ss.str();
    }
public:
    static std::string to_string(std::string const & value) {
        return value;
    }
    static std::string to_string(std::string && value) {
        return std::move(value);
    }
    static std::string to_string(const char * value) {
        SDL_ASSERT(value);
        return is_str_valid(value) ? std::string(value) : std::string();
    }
#if 0 // to be tested
    static std::string to_string(const double value) {
        if (static_cast<int>(value) == value)
            return std::to_string(static_cast<int>(value));
        return std::to_string(value);
    }
    static std::string to_string(const float value) {
        if (static_cast<int>(value) == value)
            return std::to_string(static_cast<int>(value));
        return std::to_string(value);
    }
#endif
    template<typename T>
    static std::string to_string(T && value) {
        return impl_to_string(std::forward<T>(value), 
            bool_constant<is_std_to_string<T>::value>());
    }
    template<typename T, typename... Ts>
    static std::string to_string(T && value, Ts&&... params) {
        return str_util::to_string(std::forward<T>(value)) + 
            str_util::to_string(std::forward<Ts>(params)...);
    }
};

} // sdl

#endif // __SDL_COMMON_TO_STRING_H__

