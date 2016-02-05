// output_stream.h
//
#ifndef __SDL_SYSTEM_OUTPUT_STREAM_H__
#define __SDL_SYSTEM_OUTPUT_STREAM_H__

#pragma once

#include <iostream>

namespace sdl {

#if 0 // reserved
class sdl_ostream : noncopyable {
    using ostream = decltype(std::cout);
public:
    template<class T>
    sdl_ostream & operator << (T && s) {
        std::cout << std::forward<T>(s);
        return *this;
    }
	sdl_ostream &  operator << (ostream& (*_Pfn)(ostream&)) {
        std::cout << _Pfn;
        return *this;
    }
};
#endif

class scoped_redirect: noncopyable {
    using ostream = std::ostream;
    ostream & original;
    ostream & redirect;
public:
    scoped_redirect(ostream & _original, ostream & _redirect);
    ~scoped_redirect();
};

class null_ostream : public std::ostream {
    using base = std::ostream;
public:
    null_ostream(): base(nullptr) {}
};

class scoped_null_cout_base: noncopyable {
protected:
    null_ostream null;
};

class scoped_null_cout: scoped_null_cout_base {
    scoped_redirect redirect;
public:
    scoped_null_cout(): redirect(std::cout, null) {}
};

} //namespace sdl

#endif // __SDL_SYSTEM_OUTPUT_STREAM_H__