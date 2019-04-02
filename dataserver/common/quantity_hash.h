// quantity_hash.h
//
#pragma once
#ifndef __SDL_COMMON_QUANTITY_HASH_H__
#define __SDL_COMMON_QUANTITY_HASH_H__

#include "dataserver/common/quantity.h"
#include <functional>

namespace sdl {

template <class T>
struct quantity_hash;

template <class UNIT_TYPE, class VALUE_TYPE>
struct quantity_hash<quantity<UNIT_TYPE, VALUE_TYPE>> {
    using T = quantity<UNIT_TYPE, VALUE_TYPE>;
    size_t operator()(T const & obj) const {
        return std::hash<VALUE_TYPE>()(obj.value());
    }
};

template <class T>
struct quantity_equal_to;

template <class UNIT_TYPE, class VALUE_TYPE>
struct quantity_equal_to<quantity<UNIT_TYPE, VALUE_TYPE>> {
    using T = quantity<UNIT_TYPE, VALUE_TYPE>;
    size_t operator()(T const & x, T const & y) const {
        using namespace quantity_;
        return x == y;
    }
};

} // sdl

#endif // __SDL_COMMON_QUANTITY_HASH_H__