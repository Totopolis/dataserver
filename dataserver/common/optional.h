// optional.h
#pragma once
#ifndef __SDL_COMMON_OPTIONAL_H__
#define __SDL_COMMON_OPTIONAL_H__

namespace sdl {

template<typename T>
struct optional final {
    using value_type = T;
    bool initialized = false;
    value_type value = {};
    optional() = default;
    template<typename T2>
    optional(T2 && v)
        : initialized(true)
        , value(std::forward<T2>(v)){}
    template<typename T2>
    optional & operator= (T2 && v){
        value = std::forward<T2>(v);
        initialized = true;
        return *this;
    }
    explicit operator bool() const {
        return initialized;
    }
private: // reserved
    bool operator == (T const & v) const {
        return initialized && (value == v);
    }
};

} // sdl

#endif // __SDL_COMMON_OPTIONAL_H__

