// singleton.h
//
#pragma once
#ifndef __SDL_COMMON_SINGLETON_H__
#define __SDL_COMMON_SINGLETON_H__

namespace sdl {

template <class T>
class singleton : private T {
    singleton() = default;
    ~singleton() = default;
public:
    static T & instance() {
        static singleton<T> object;
        return object;
    }
    static T const & cinstance() {
		return instance();
	}
};

} // sdl

#endif // __SDL_COMMON_SINGLETON_H__
