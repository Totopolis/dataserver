// config.h
//
#pragma once
#ifndef __SDL_COMMON_CONFIG_H__
#define __SDL_COMMON_CONFIG_H__

#if SDL_DEBUG
#include <assert.h>
#endif

#if SDL_DEBUG
namespace sdl {
    struct debug {
        static void warning(const char * message, const char * fun, const int line);
        static void trace() {
            std::cout << std::endl;
        }
        template<typename T, typename... Ts>
        static void trace(T && value, Ts&&... params) {
            std::cout << value; trace(params...);
        }
        static int warning_level;
    };
}
#define SDL_ASSERT(x)               assert(x)
#define SDL_WARNING(x)              (void)((x) || (sdl::debug::warning(#x, __FUNCTION__, __LINE__), 0))
#define SDL_VERIFY(expr)            (void)((expr) || (assert(false), 0))
#define SDL_TRACE(...)              sdl::debug::trace(__VA_ARGS__)
#define SDL_TRACE_FILE              ((void)0)
//#define SDL_TRACE_FILE            SDL_TRACE(__FILE__)
#else
#define SDL_ASSERT(x)               ((void)0)
#define SDL_WARNING(x)              ((void)0)
#define SDL_VERIFY(expr)            ((void)(expr))
#define SDL_TRACE(...)              ((void)0)
#define SDL_TRACE_FILE              ((void)0)
#endif

#define CURRENT_BYTE_ORDER          (*(int *)"\x01\x02\x03\x04")
#define LITTLE_ENDIAN_BYTE_ORDER    0x04030201
#define BIG_ENDIAN_BYTE_ORDER       0x01020304
#define PDP_ENDIAN_BYTE_ORDER       0x02010403

#define IS_LITTLE_ENDIAN (CURRENT_BYTE_ORDER == LITTLE_ENDIAN_BYTE_ORDER)
#define IS_BIG_ENDIAN    (CURRENT_BYTE_ORDER == BIG_ENDIAN_BYTE_ORDER)
#define IS_PDP_ENDIAN    (CURRENT_BYTE_ORDER == PDP_ENDIAN_BYTE_ORDER)

#define A_STATIC_ASSERT_IS_POD(x)           static_assert(std::is_pod<x>::value, "std::is_pod")
#define A_STATIC_ASSERT_TYPE(T1, T2)        static_assert(std::is_same<T1, T2>::value, "std::is_same")
#define A_STATIC_ASSERT_NOT_TYPE(T1, T2)    static_assert(!std::is_same<T1, T2>::value, "!std::is_same")
#define A_STATIC_SAME_TYPE(x1, x2)          static_assert(std::is_same<decltype(x1), decltype(x2)>::value, "std::is_same decltype")
#define A_STATIC_NOT_SAME_TYPE(x1, x2)      static_assert(!std::is_same<decltype(x1), decltype(x2)>::value, "!std::is_same decltype")
#define A_STATIC_CHECK_TYPE(T, x)           static_assert(std::is_same<T, decltype(x)>::value, "std::is_same")

#define A_STATIC_ASSERT_64_BIT \
    static_assert(sizeof(void *) == sizeof(std::int64_t), "64-bit only"); \
    static_assert(sizeof(size_t) == sizeof(std::int64_t), "64-bit only")

#endif // __SDL_COMMON_CONFIG_H__