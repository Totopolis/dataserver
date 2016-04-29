// static.h
//
#pragma once
#ifndef __SDL_COMMON_STATIC_H__
#define __SDL_COMMON_STATIC_H__

#include "config.h"
#include <memory>
#include <stdio.h> // for sprintf_s
#include <cstdio> // for snprintf 
#include <cstring> // for memset
#include <exception>
#include <stdexcept>
#include <algorithm>
#include <cmath> // std::fabs
#include <cstdlib> // std::mbstowcs

namespace sdl {

using uint8 = std::uint8_t;
using int16 = std::int16_t;
using uint16 = std::uint16_t;
using int32 = std::int32_t;
using uint32 = std::uint32_t;
using int64 = std::int64_t;
using uint64 = std::uint64_t;

inline bool is_str_empty(const char * str)
{
    return !(str && str[0]);
}

inline bool is_str_empty(const wchar_t * str)
{
    return !(str && str[0]);
}

inline bool is_str_valid(const char * str)
{
    return str && str[0];
}

inline bool is_str_valid(const wchar_t * str)
{
    return str && str[0];
}

template <class T> inline T a_min(const T a, const T b)
{
    static_assert(sizeof(T) <= sizeof(double), "");
    return (a < b) ? a : b;
}

template <class T> inline T a_max(const T a, const T b)
{
    static_assert(sizeof(T) <= sizeof(double), "");
    return (b < a) ? a : b;
}

template <class T> inline T a_abs(const T a)
{
    static_assert(sizeof(T) <= sizeof(double), "");
    return (a < 0) ? (-a) : a;
}

template <typename T> inline bool fequal(T f1, T f2)
{
    static_assert(std::is_same<float, T>::value || std::is_same<double, T>::value, "");
    return std::fabs(f1 - f2) <= 1e-12;
}

template<class T> inline void memset_pod(T& dest, int value)
{
    A_STATIC_ASSERT_IS_POD(T);
    memset(&dest, value, sizeof(T));
}

template<class T> inline void memset_zero(T& dest)
{
    A_STATIC_ASSERT_IS_POD(T);
    memset(&dest, 0, sizeof(T));
}

template<class Dest, class Source> inline 
void memcpy_pod(Dest & dest, Source const & src)
{
    A_STATIC_ASSERT_IS_POD(Dest);
    A_STATIC_ASSERT_IS_POD(Source);
    static_assert(sizeof(Dest) == sizeof(Source), "");
    memcpy(&dest, &src, sizeof(dest));
}

template<class T1, class T2> inline 
int memcmp_pod(T1 const & x, T2 const & y)
{
    A_STATIC_ASSERT_IS_POD(T1);
    A_STATIC_ASSERT_IS_POD(T2);
    static_assert(sizeof(T1) == sizeof(T2), "");
    return memcmp(&x, &y, sizeof(x));
}

template<unsigned int v> struct is_power_2
{
    enum { value = v && !(v & (v - 1)) };
};

template<size_t N> struct kilobyte
{
    enum { value = N * (1 << 10) };
};

template<size_t N> struct megabyte
{
    enum { value = N * (1 << 20) };
};

template<size_t N> struct min_to_sec
{
    enum { value = N * 60 };
};

template<size_t N> struct hour_to_sec
{
    enum { value = N * 60 * 60 };
};

template<size_t N> struct day_to_sec
{
    enum { value = 24 * hour_to_sec<N>::value };
};

// This template function declaration is used in defining A_ARRAY_SIZE.
// Note that the function doesn't need an implementation, as we only use its type.
template <typename T, size_t N> char(&A_ARRAY_SIZE_HELPER(T const(&array)[N]))[N];
#define A_ARRAY_SIZE(a) (sizeof(A_ARRAY_SIZE_HELPER(a)))

template <typename T> struct array_info;
template <typename T, size_t N>
struct array_info<T[N]>
{
    typedef T value_type;
    enum { size = N };
};

//Alternative of macros A_ARRAY_SIZE
//This function template can't be instantiated with pointer argument, only array.
template< class Type, size_t n > inline // constexpr 
size_t count_of(Type const(&)[n])
{
    return n;
}


template<size_t buf_size, typename... Ts> inline
const char * format_s(char(&buf)[buf_size], Ts&&... params) {
    if (
#if defined(SDL_OS_WIN32)
    #pragma warning(push)
    #pragma warning(disable: 4996) // for _snprintf
        _snprintf
    #pragma warning(pop)
#else
        snprintf
#endif
        (buf, buf_size, std::forward<Ts>(params)...) > 0) {
        buf[buf_size-1] = 0;
        return buf;
    }
    SDL_ASSERT(!"format_s");
    buf[0] = 0;
    return buf;
}


template <unsigned long N> struct binary;

template <> struct binary<0>
{
    static unsigned const value = 0;
};

template <unsigned long N>
struct binary
{
    static unsigned const value = 
        (binary<N / 10>::value << 1)    // prepend higher bits
            | (N % 10);                 // to lowest bit
};

// std::make_unique available since C++14
template<typename T, typename... Ts> inline
std::unique_ptr<T> make_unique(Ts&&... params) {
    return std::unique_ptr<T>(new T(std::forward<Ts>(params)...));
}

template<typename pointer, typename... Ts> inline
void reset_new(pointer & dest, Ts&&... params) {
    using T = typename pointer::element_type;
    dest.reset(new T(std::forward<Ts>(params)...));
}

template<class T, class Base, typename... Ts> inline
std::shared_ptr<Base> & reset_shared(std::shared_ptr<Base> & dest, Ts&&... params) {
    static_assert(std::is_base_of<Base, T>::value, "");
    static_assert(std::has_virtual_destructor<Base>::value, "");
    dest.reset(new T(std::forward<Ts>(params)...));
    return dest;
}

class sdl_exception : public std::logic_error {
    using base_type = std::logic_error;
public:
    sdl_exception(): base_type("unknown exception"){}
    explicit sdl_exception(const char* s): base_type(s){
        SDL_ASSERT(s);
    }
};

template<class T>
class sdl_exception_t : public sdl_exception {
public:
    using type = T;
    explicit sdl_exception_t(const char* s) : sdl_exception(s){}
};

template<typename T, typename... Ts> inline
void throw_error(Ts&&... params) {
    static_assert(std::is_base_of<sdl_exception, T>::value, "is_base_of");
    SDL_ASSERT(!"throw_error");
    throw T(std::forward<Ts>(params)...);
}

template<typename T, typename... Ts> inline
void throw_error_if(const bool condition, Ts&&... params) {
    static_assert(std::is_base_of<sdl_exception, T>::value, "is_base_of");
    if (condition) {
        SDL_ASSERT(!"throw_error_if");
        throw T(std::forward<Ts>(params)...);
    }
}

template<class T> using vector_unique_ptr = std::vector<std::unique_ptr<T>>;
template<class T> using vector_shared_ptr = std::vector<std::shared_ptr<T>>;

template<class T>
using remove_reference_t = typename std::remove_reference<T>::type; // since C++14

template <typename T> struct identity
{
    typedef T type;
};

template<typename T> 
inline bool is_found(T && key, std::initializer_list<T> a) {
    return std::find(a.begin(), a.end(), key) != a.end();
}

template<class T, class fun_type>
void for_reverse(T && data, fun_type fun) {
    auto const last = data.begin();
    auto it = data.end();
    if (it != last) {
        do {
            --it;
            fun(*it);
        } while (it != last);
    }
}

// https://en.wikipedia.org/wiki/Windows-1251
inline std::wstring cp1251_to_wide(std::string const & s) {
    std::wstring w(s.size(), L'\0');
    if (std::mbstowcs(&w[0], s.c_str(), w.size()) == s.size()) {
        return w;
    }
    SDL_ASSERT(!"cp1251_to_wide");
    return{};
}

} // sdl

#endif // __SDL_COMMON_STATIC_H__
