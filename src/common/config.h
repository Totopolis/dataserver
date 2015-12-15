// config.h
//
#ifndef __SDL_COMMON_CONFIG_H__
#define __SDL_COMMON_CONFIG_H__

#pragma once

#if SDL_DEBUG
#include <assert.h>
#endif

#if SDL_DEBUG
#define SDL_TRACE(x)                      { std::cout << (x) << std::endl; }
#define SDL_TRACE_2(x, y)                 { std::cout << (x) << (y) << std::endl; }
#define SDL_TRACE_3(x1, x2, x3)           { std::cout << (x1) << (x2) << (x3) << std::endl; }
#define SDL_TRACE_4(x1, x2, x3, x4)       { std::cout << (x1) << (x2) << (x3) << (x4) << std::endl; }
#define SDL_TRACE_5(x1, x2, x3, x4, x5)   { std::cout << (x1) << (x2) << (x3) << (x4) << (x5) << std::endl; }
#else
#define SDL_TRACE(x)
#define SDL_TRACE_2(x, y)
#define SDL_TRACE_3(x1, x2, x3)
#define SDL_TRACE_4(x1, x2, x3, x4)
#define SDL_TRACE_5(x1, x2, x3, x4, x5)
#endif

#if SDL_DEBUG
namespace sdl {
    namespace debug {
        inline void warning(const char * fun, int line) {
            std::cout << "\nwarning in " << fun << " at line " << line << std::endl; 
        }
    }
}
#define SDL_ASSERT(x)               assert(x)
#define SDL_WARNING(x)              if (!(x)) { sdl::debug::warning(__FUNCTION__, __LINE__); }
#else
#define SDL_ASSERT(x)
#define SDL_WARNING(x)
#endif

#if SDL_DEBUG
#define SDL_VERIFY(expr)			if (!(expr)) { SDL_ASSERT(false); } 
#define SDL_VERIFY_WARNING(expr)	if (!(expr)) { SDL_WARNING(false); } 
#else
#define SDL_VERIFY(expr)			((void)(expr))
#define SDL_VERIFY_WARNING(expr)	((void)(expr))
#endif

#define CURRENT_BYTE_ORDER			(*(int *)"\x01\x02\x03\x04")
#define LITTLE_ENDIAN_BYTE_ORDER	0x04030201
#define BIG_ENDIAN_BYTE_ORDER		0x01020304
#define PDP_ENDIAN_BYTE_ORDER		0x02010403

#define IS_LITTLE_ENDIAN (CURRENT_BYTE_ORDER == LITTLE_ENDIAN_BYTE_ORDER)
#define IS_BIG_ENDIAN    (CURRENT_BYTE_ORDER == BIG_ENDIAN_BYTE_ORDER)
#define IS_PDP_ENDIAN    (CURRENT_BYTE_ORDER == PDP_ENDIAN_BYTE_ORDER)

#define A_STATIC_ASSERT_IS_POD(x)	    static_assert(std::is_pod<x>::value, "std::is_pod")
#define A_STATIC_ASSERT_TYPE(T1, T2)    static_assert(std::is_same<T1, T2>::value, "std::is_same")
#define A_STATIC_SAME_TYPE(x1, x2)      static_assert(std::is_same<decltype(x1), decltype(x2)>::value, "std::is_same decltype")
#define A_STATIC_NOT_SAME_TYPE(x1, x2)  static_assert(!std::is_same<decltype(x1), decltype(x2)>::value, "!std::is_same decltype")
#define A_STATIC_CHECK_TYPE(T, x)       static_assert(std::is_same<T, decltype(x)>::value, "std::is_same")

#define A_STATIC_ASSERT_64_BIT \
    static_assert(sizeof(void *) == sizeof(std::int64_t), "64-bit only"); \
    static_assert(sizeof(size_t) == sizeof(std::int64_t), "64-bit only")

#endif // __SDL_COMMON_CONFIG_H__