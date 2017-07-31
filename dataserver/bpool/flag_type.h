// flag_type.h
//
#pragma once
#ifndef __SDL_BPOOL_FLAG_TYPE_H__
#define __SDL_BPOOL_FLAG_TYPE_H__

namespace sdl { namespace db { namespace bpool {

//----------------------------------------------------------

enum class fixedf { false_, true_ };

inline constexpr fixedf make_fixedf(bool b) {
    return static_cast<fixedf>(b);
}
inline constexpr bool is_fixed(fixedf f) {
    return fixedf::false_ != f;
}

//----------------------------------------------------------

enum class removef { false_, true_ };

inline constexpr removef make_removef(bool b) {
    return static_cast<removef>(b);
}
inline constexpr bool is_remove(removef f) {
    return removef::false_ != f;
}

//----------------------------------------------------------

enum class decommitf { false_, true_ };

inline constexpr decommitf make_decommitf(bool b) {
    return static_cast<decommitf>(b);
}
inline constexpr bool is_decommit(decommitf f) {
    return decommitf::false_ != f;
}

//----------------------------------------------------------

}}} // sdl

#endif // __SDL_BPOOL_FLAG_TYPE_H__
