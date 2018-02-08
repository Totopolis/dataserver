// bitmask.h
#pragma once
#ifndef __SDL_COMMON_BITMASK_H__
#define __SDL_COMMON_BITMASK_H__

#include "dataserver/common/common.h"

namespace sdl {

template<typename MASK_TYPE, class ENUM_TYPE>
struct bitmask_t
{
    using mask_type = MASK_TYPE;
    using enum_type = ENUM_TYPE;
    enum { dim = 8 * sizeof(mask_type) };
    mask_type mask;

    explicit operator bool() const {
        return !mask;
    }
    bool bit(enum_type const i) const {
        SDL_ASSERT(int(i) >= 0);
        SDL_ASSERT(int(i) < dim);
        static_assert(mask_type(-1) > 0, "");
        return (mask & (mask_type(1) << int(i))) != 0;
    }
    void set_bit(enum_type const i, const bool flag) {
        SDL_ASSERT(int(i) >= 0);
        SDL_ASSERT(int(i) < dim);
        static_assert(mask_type(-1) > 0, "");
        if (flag) {
            mask |= (mask_type(1) << int(i));
        }
        else {
            mask &= ~(mask_type(1) << int(i));
        }
    }
    void set_bit(enum_type const i) {
        SDL_ASSERT(int(i) >= 0);
        SDL_ASSERT(int(i) < dim);
        static_assert(mask_type(-1) > 0, "");
        mask |= (mask_type(1) << int(i));
    }
    template<enum_type i> 
    bool bit() const { 
        static_assert(size_t(i) < dim, "");
        return bit(i);
    }
    template<enum_type i> 
    void set_bit(const bool flag) {
        static_assert(size_t(i) < dim, "");
        set_bit(i, flag);
    }
    template<enum_type i> 
    void set_bit() {
        static_assert(size_t(i) < dim, "");
        set_bit(i);
    }
};

#if SDL_DEBUG 
template<typename MASK_TYPE, class ENUM_TYPE>
std::ostream & operator <<(std::ostream & out, bitmask_t<MASK_TYPE, ENUM_TYPE> const & src) {
    using T = bitmask_t<MASK_TYPE, ENUM_TYPE>;
    for (int i = 0; i < T::dim; ++i) {
        const bool b = src.bit(static_cast<ENUM_TYPE>(i));
        out << (b ? "1" : "0");
    }
    return out;
}
#endif

} // sdl

#endif // __SDL_COMMON_BITMASK_H__