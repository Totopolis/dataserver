// array_enum.h
//
#pragma once
#ifndef __SDL_COMMON_ARRAY_ENUM_H__
#define __SDL_COMMON_ARRAY_ENUM_H__

#include "dataserver/common/static.h"

namespace sdl {

template<typename T, size_t N, class enum_type>
struct array_enum_t
{
    using type = T;
    static const size_t size = N;
    T elem[N];

    T const & operator[](enum_type t) const {
        SDL_ASSERT(size_t(t) < N);
        return elem[size_t(t)];
    }
    T & operator[](enum_type t) noexcept {
        SDL_ASSERT(size_t(t) < N);
        return elem[size_t(t)];
    }
    T const * begin() const noexcept {
        return std::begin(elem);
    }
    T const * end() const noexcept {
        return std::end(elem);
    }
    T * begin() noexcept {
        return std::begin(elem);
    }
    T * end() noexcept {
        return std::end(elem);
    }
};

template<typename mask_type, class enum_type>
struct bitmask_enum_t
{
    mask_type mask;

    bool bit(enum_type const i) const {
        SDL_ASSERT(int(i) >= 0);
        SDL_ASSERT(int(i) < (int)(8 * sizeof(mask)));
        static_assert(std::is_unsigned<mask_type>::value, "");
        static_assert(mask_type(-1) > 0, "");
        return (mask & (mask_type(1) << int(i))) != 0;
    }
    void set_bit(enum_type const i, const bool flag) {
        SDL_ASSERT(int(i) >= 0);
        SDL_ASSERT(int(i) < (int)(8 * sizeof(mask)));
        static_assert(std::is_unsigned<mask_type>::value, "");
        static_assert(mask_type(-1) > 0, "");
        if (flag) {
            mask |= (mask_type(1) << static_cast<int>(i));
        }
        else {
            mask &= ~(mask_type(1) << static_cast<int>(i));
        }
    }
    template<enum_type i> 
    bool bit() const { 
        static_assert(size_t(i) < 8 * sizeof(mask), "bit");
        return bit(i);
    }
    template<enum_type i> 
    void set_bit(const bool flag = true) {
        static_assert(size_t(i) < 8 * sizeof(mask), "set_bit");
        set_bit(i, flag);
    }
};

template<typename mask_type>
using bitmask_t = bitmask_enum_t<mask_type, int>;

} // sdl

#endif // __SDL_COMMON_ARRAY_ENUM_H__
