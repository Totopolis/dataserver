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

template<typename T>
struct bitmask : is_static
{
    enum { max_bit = 8 * sizeof(T) };
    static bool is_bit(T const mask, size_t const i) {
        SDL_ASSERT(i < max_bit);
        static_assert(std::is_unsigned<T>::value, "");
        return (mask & (static_cast<T>(1) << i)) != 0;
    }
    template<size_t i>
    static bool is_bit(T const mask) {
        static_assert(i < max_bit, "is_bit");
        static_assert(std::is_unsigned<T>::value, "");
        return (mask & (static_cast<T>(1) << i)) != 0;
    }
    static void set_bit(T & mask, size_t const i) {
        SDL_ASSERT(i < max_bit);
        mask |= (static_cast<T>(1) << i);
    }
    template<size_t i>
    static void set_bit(T & mask) {
        static_assert(i < max_bit, "set_bit");
        mask |= (static_cast<T>(1) << i);
    }
    static void clr_bit(T & mask, size_t const i) {
        SDL_ASSERT(i < max_bit);
        mask &= ~(static_cast<T>(1) << i);
    }
    template<size_t i>
    static void clr_bit(T & mask) {
        static_assert(i < max_bit, "clr_bit");
        mask &= ~(static_cast<T>(1) << i);
    }
};

template<typename mask_type, class enum_type>
struct bitmask_enum_t
{
    mask_type mask;

    bool is_bit(enum_type const i) const {
        return bitmask<mask_type>::is_bit(mask, (size_t)i);
    }
    template<enum_type i> 
    bool is_bit() const {
        static_assert(static_cast<size_t>(i) < bitmask<mask_type>::max_bit, "");
        return is_bit(i);
    }
    void set_bit(enum_type const i) {
        bitmask<mask_type>::set_bit(mask, (size_t)i);
    }
    template<enum_type i> 
    void set_bit() {
        static_assert(static_cast<size_t>(i) < bitmask<mask_type>::max_bit, "");
        set_bit(i);
    }
    void clr_bit(enum_type const i) {
        bitmask<mask_type>::clr_bit(mask, (size_t)i);
    }
    template<enum_type i> 
    void clr_bit() {
        static_assert(static_cast<size_t>(i) < bitmask<mask_type>::max_bit, "");
        clr_bit(i);
    }
};

} // sdl

#endif // __SDL_COMMON_ARRAY_ENUM_H__
