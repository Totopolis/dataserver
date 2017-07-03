// hash_combine.h
//
#pragma once
#ifndef __SDL_COMMON_HASH_COMBINE_H__
#define __SDL_COMMON_HASH_COMBINE_H__

#include "dataserver/common/common.h"

namespace sdl { namespace hash_detail {

/*#if defined(_MSC_VER)
#   define SDL_FUNCTIONAL_HASH_ROTL32(x, r) _rotl(x,r)
#else
#   define SDL_FUNCTIONAL_HASH_ROTL32(x, r) (x << r) | (x >> (32 - r))
#endif*/

template<uint32 r>
inline constexpr uint32 hash_rotl32(const uint32 x)
{
    return (x << r) | (x >> (32 - r));
}

inline void hash_combine_impl(uint64 & h, uint64 k)
{
    constexpr uint64_t m = UINT64_C(0xc6a4a7935bd1e995);
    constexpr int r = 47;

    k *= m;
    k ^= k >> r;
    k *= m;

    h ^= k;
    h *= m;

    // Completely arbitrary number, to prevent 0's
    // from hashing to 0.
    h += 0xe6546b64;
}

template <typename SizeT>
inline void hash_combine_impl(SizeT & seed, SizeT value)
{
    seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

inline void hash_combine_impl(uint32 & h1, uint32 k1)
{
    constexpr uint32 c1 = 0xcc9e2d51;
    constexpr uint32 c2 = 0x1b873593;

    k1 *= c1;
    k1 = hash_rotl32<15>(k1);
    k1 *= c2;

    h1 ^= k1;
    h1 = hash_rotl32<13>(h1);
    h1 = h1 * 5 + 0xe6546b64;
}

template <class T>
inline void hash_combine(std::size_t & seed, T const & v) // see boost::hash_combine
{
    hash_combine_impl(seed, std::hash<T>{}(v));
}

} // hash_detail
} // sdl

#endif // __SDL_COMMON_HASH_COMBINE_H__
