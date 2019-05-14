// config.h
//
#pragma once
#ifndef __SDL_COMMON_CONFIG_H__
#define __SDL_COMMON_CONFIG_H__

#include "dataserver/common/stdcommon.h"

#if SDL_DEBUG || defined(SDL_TRACE_RELEASE)
#define SDL_TRACE_ENABLED   1
#else
#define SDL_TRACE_ENABLED   0
#endif

namespace sdl {
#if SDL_TRACE_ENABLED
    struct no_endl{};
    struct debug {
        static int & warning_level();
        static bool & is_unit_test();
        static bool & is_print_timestamp();
        static void trace(no_endl) {}
        static void trace();
        static void print_timestamp();
        static void print_timestampw();
        static void trace_with_timestamp();
        static void tracew_with_timestamp();

        template<typename... Ts>
        static void trace(decltype(nullptr), Ts&&... params) {
            std::cout << "nullptr";
            trace(std::forward<Ts>(params)...);
        }
        template<typename T, typename... Ts>
        static void trace(T && value, Ts&&... params) {
            std::cout << value;
            trace(std::forward<Ts>(params)...);
        }
        static void tracew() { std::wcout << std::endl; }
        template<typename... Ts>
        static void tracew(decltype(nullptr), Ts&&... params) {
            std::wcout << "nullptr";
            tracew(std::forward<Ts>(params)...);
        }
        template<typename T, typename... Ts>
        static void tracew(T && value, Ts&&... params) {
            std::wcout << value;
            tracew(std::forward<Ts>(params)...);
        }
        template<typename T, typename... Ts>
        static void trace_with_timestamp(T && value, Ts&&... params) {
            print_timestamp();
            trace(std::forward<T>(value), std::forward<Ts>(params)...);
        }
        template<typename T, typename... Ts>
        static void tracew_with_timestamp(T && value, Ts&&... params) {
            print_timestampw();
            tracew(std::forward<T>(value), std::forward<Ts>(params)...);
        }
        template<typename T, typename... Ts>
        static void trace_if(const bool condition, T && value, Ts&&... params) {
            if (condition) {
                trace(std::forward<T>(value), std::forward<Ts>(params)...);
            }
        }
        template<typename T, typename... Ts>
        static void trace_if_with_timestamp(const bool condition, T && value, Ts&&... params) {
            if (condition) {
                trace_with_timestamp(std::forward<T>(value), std::forward<Ts>(params)...);
            }
        }
        static void warning(const char * message, const char * fun, const int line);
    };
#endif
} // sdl

#if SDL_TRACE_ENABLED
#define SDL_TRACE(...)              sdl::debug::trace_with_timestamp(__VA_ARGS__)
#define SDL_TRACEW(...)             sdl::debug::tracew_with_timestamp(__VA_ARGS__)
#define SDL_TRACE_IF(...)           sdl::debug::trace_if_with_timestamp(__VA_ARGS__)
#define SDL_TRACE_FILE              ((void)0)
#define SDL_TRACE_FUNCTION          SDL_TRACE(__FUNCTION__)
#define SDL_DEBUG_SETPRECISION(...) std::cout << std::setprecision(__VA_ARGS__)
#define SDL_TRACE_UTF8(...)         SDL_TRACEW(sdl::db::conv::utf8_to_wide(__VA_ARGS__))
#else
#define SDL_TRACE(...)              ((void)0)
#define SDL_TRACEW(...)             ((void)0)
#define SDL_TRACE_IF(...)           ((void)0)
#define SDL_TRACE_FILE              ((void)0)
#define SDL_TRACE_FUNCTION          ((void)0)
#define SDL_DEBUG_SETPRECISION(...) ((void)0)
#define SDL_TRACE_UTF8(...)         ((void)0)
#endif

#if SDL_DEBUG && defined(NDEBUG) 
#if defined(SDL_OS_WIN32)
#define SDL_NDEBUG_ASSERT(x) (void)(!!(x) || (sdl::debug::warning(#x, __FUNCTION__, __LINE__), __debugbreak(), 0))
#else
#define SDL_NDEBUG_ASSERT(x) (void)(!!(x) || (sdl::debug::warning(#x, __FUNCTION__, __LINE__), __builtin_trap(), 0))   
#endif
#endif

#if SDL_DEBUG
#if defined(SDL_OS_WIN32) && defined(NDEBUG) 
inline void SDL_ASSERT_1(bool x)    { SDL_NDEBUG_ASSERT(x); }
#define SDL_ASSERT(...)             SDL_NDEBUG_ASSERT(__VA_ARGS__)
#else
inline void SDL_ASSERT_1(bool x)    { assert(x); }
#define SDL_ASSERT(...)             assert(__VA_ARGS__)
#endif
#define SDL_WARNING(x)              (void)(!!(x) || (sdl::debug::warning(#x, __FUNCTION__, __LINE__), 0))
#define SDL_VERIFY(expr)            (void)(!!(expr) || (assert(false), 0))
#define SDL_DEBUG_CPP(expr)         expr
#define SDL_DEBUG_HPP(expr)         expr
#else
#define SDL_ASSERT_1(...)           ((void)0)
#define SDL_ASSERT(...)             ((void)0)
#define SDL_WARNING(...)            ((void)0)
#define SDL_VERIFY(...)             ((void)(expr))
#define SDL_DEBUG_CPP(...)          ((void)0)
#define SDL_DEBUG_HPP(...)
#endif
#define SDL_ASSERT_DISABLED(...)    ((void)0)
#define SDL_STATIC_ASSERT(x)        static_assert(!!(x), #x)

#if SDL_DEBUG > 1
#define SDL_ASSERT_DEBUG_2(...)     SDL_ASSERT(__VA_ARGS__)
#define SDL_WARNING_DEBUG_2(...)    SDL_WARNING(__VA_ARGS__)
#define SDL_TRACE_DEBUG_2(...)      SDL_TRACE(__VA_ARGS__)
#define SDL_DEBUG_CPP_2(expr)       expr
#define SDL_DEBUG_HPP_2(expr)       expr
#else
#define SDL_ASSERT_DEBUG_2(...)     ((void)0)
#define SDL_WARNING_DEBUG_2(...)    ((void)0)
#define SDL_TRACE_DEBUG_2(...)      ((void)0)
#define SDL_DEBUG_CPP_2(...)        ((void)0)
#define SDL_DEBUG_HPP_2(...)
#endif

#if defined(SDL_OS_WIN32)
#define SDL_ASSERT_WIN32(...)       SDL_ASSERT(__VA_ARGS__)
#else
#define SDL_ASSERT_WIN32(...)       ((void)0)
#endif

#if SDL_DEBUG
#define SDL_TRACE_ERROR(...)        (SDL_TRACE(__VA_ARGS__), SDL_ASSERT(0), 0)
#define SDL_TRACE_WARNING(...)      (SDL_TRACE(__VA_ARGS__), SDL_WARNING(0), 0)
#else
#define SDL_TRACE_ERROR(...)        ((void)0)
#define SDL_TRACE_WARNING(...)      ((void)0)
#endif

#if SDL_DEBUG
#define SDL_NDEBUG_CONSTEXPR
#else
#define SDL_NDEBUG_CONSTEXPR        constexpr
#endif

#define CURRENT_BYTE_ORDER          (*(uint32 *)"\x01\x02\x03\x04")
#define LITTLE_ENDIAN_BYTE_ORDER    0x04030201
#define BIG_ENDIAN_BYTE_ORDER       0x01020304
#define PDP_ENDIAN_BYTE_ORDER       0x02010403

#define IS_LITTLE_ENDIAN (CURRENT_BYTE_ORDER == LITTLE_ENDIAN_BYTE_ORDER)
#define IS_BIG_ENDIAN    (CURRENT_BYTE_ORDER == BIG_ENDIAN_BYTE_ORDER)
#define IS_PDP_ENDIAN    (CURRENT_BYTE_ORDER == PDP_ENDIAN_BYTE_ORDER)

#define A_STATIC_ASSERT_IS_POD(...)         static_assert(std::is_pod<__VA_ARGS__>::value, "std::is_pod")
#define A_STATIC_ASSERT_NOT_POD(...)        static_assert(!std::is_pod<__VA_ARGS__>::value, "!std::is_pod")
#define A_STATIC_CHECK_IS_POD(...)          static_assert(std::is_pod<decltype(__VA_ARGS__)>::value, "std::is_pod")
#define A_STATIC_ASSERT_IS_INTEGRAL(...)    static_assert(std::is_integral<__VA_ARGS__>::value, "std::is_integral")
#define A_STATIC_ASSERT_TYPE(...)           static_assert(std::is_same<__VA_ARGS__>::value, "std::is_same")
#define A_STATIC_ASSERT_NOT_TYPE(...)       static_assert(!std::is_same<__VA_ARGS__>::value, "!std::is_same")
#define A_STATIC_SAME_TYPE(x1, x2)          static_assert(std::is_same<decltype(x1), decltype(x2)>::value, "std::is_same decltype")
#define A_STATIC_NOT_SAME_TYPE(x1, x2)      static_assert(!std::is_same<decltype(x1), decltype(x2)>::value, "!std::is_same decltype")
#define A_STATIC_CHECK_TYPE(T, x)           static_assert(std::is_same<T, decltype(x)>::value, "std::is_same")
#define A_STATIC_CHECK_NOT_TYPE(T, x)       static_assert(!std::is_same<T, decltype(x)>::value, "!std::is_same")

// require clang version 3.5.0 or later
#define static_assert_is_nothrow_move_assignable(x) static_assert(std::is_nothrow_move_assignable<x>::value, "std::is_nothrow_move_assignable")
#define static_check_is_nothrow_move_assignable(x)  static_assert(std::is_nothrow_move_assignable<decltype(x)>::value, "std::is_nothrow_move_assignable")
#define static_assert_is_nothrow_copy_assignable(x) static_assert(std::is_nothrow_copy_assignable<x>::value, "std::is_nothrow_copy_assignable")
#define static_check_is_nothrow_copy_assignable(x)  static_assert(std::is_nothrow_copy_assignable<decltype(x)>::value, "std::is_nothrow_copy_assignable")

#if defined(SDL_OS_WIN32) // FIXME: clang support on Linux
#define static_assert_is_trivially_copyable(x)      static_assert(std::is_trivially_copyable<x>::value, "std::is_trivially_copyable")
#else
#define static_assert_is_trivially_copyable(x)      ((void)0)
#endif

#if 0 // test for 32 bit
#define A_STATIC_ASSERT_64_BIT  ((void)0)
#else
#define A_STATIC_ASSERT_64_BIT \
    static_assert(sizeof(void *) == sizeof(std::uint64_t), "64-bit only"); \
    static_assert(sizeof(size_t) == sizeof(std::uint64_t), "64-bit only"); \
    static_assert(sizeof(void *) == 8, "64-bit only"); \
    static_assert(sizeof(size_t) == 8, "64-bit only")
#endif

#if defined(SDL_OS_WIN32) // Level4 (/W4)
#pragma warning(disable: 4127) // conditional expression is constant
#pragma warning(disable: 4512) // assignment operator could not be generated
#pragma warning(disable: 4706) // assignment within conditional expression
#endif

#if !defined(SDL_OS_WIN32)
    #if defined(NDEBUG) && SDL_DEBUG
        #error SDL_DEBUG
    #endif
#endif

#if !defined(_MSC_VER)
#ifndef __cplusplus
  #error C++ is required
#elif __cplusplus <= 199711L
  #error This library needs at least a C++11 compliant compiler
#elif __cplusplus < 201402L
  //FIXME: #error C++14 is required
#endif
#endif

#endif // __SDL_COMMON_CONFIG_H__
