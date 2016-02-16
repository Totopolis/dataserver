// output_stream.h
//
#ifndef __SDL_SYSTEM_OUTPUT_STREAM_H__
#define __SDL_SYSTEM_OUTPUT_STREAM_H__

#pragma once

#include <iostream>

namespace sdl {

template<class ostream>
class forward_ostream : noncopyable {
    ostream & ss;
public:
    explicit forward_ostream(ostream & _ss) : ss(_ss) {}
    template<class T>
    forward_ostream & operator << (T && s) {
        ss << std::forward<T>(s);
        return *this;
    }
    forward_ostream & operator << (ostream& (*_Pfn)(ostream&)) {
        ss << _Pfn;
        return *this;
    }
};

template<class ostream>
class scoped_redirect: noncopyable {
    ostream & original;
    ostream & redirect;
public:
    scoped_redirect(ostream & _original, ostream & _redirect)
        : original(_original)
        , redirect(_redirect) {
        original.rdbuf(redirect.rdbuf(original.rdbuf()));
    }
    ~scoped_redirect() {
        original.rdbuf(redirect.rdbuf(original.rdbuf()));
    }
};

template<class ostream>
class null_ostream : public ostream {
public:
    null_ostream(): ostream(nullptr) {}
};

template<class ostream>
class scoped_null_cout_base: noncopyable {
protected:
    null_ostream<ostream> null;
};

template<class ostream>
class scoped_null_t: scoped_null_cout_base<ostream> {
    using base = scoped_null_cout_base<ostream>;
    scoped_redirect<ostream> redirect;
public:
    explicit scoped_null_t(ostream & s): redirect(s, base::null) {}
};

class scoped_null_cout: scoped_null_t<std::ostream> {
    using base = scoped_null_t<std::ostream>;
public:
    scoped_null_cout(): base(std::cout) {}
};

class scoped_null_wcout: scoped_null_t<std::wostream> {
    using base = scoped_null_t<std::wostream>;
public:
    scoped_null_wcout(): base(std::wcout) {}
};

} //namespace sdl

#endif // __SDL_SYSTEM_OUTPUT_STREAM_H__