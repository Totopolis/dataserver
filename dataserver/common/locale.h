// locale.h
#pragma once
#ifndef __SDL_COMMON_LOCALE_H__
#define __SDL_COMMON_LOCALE_H__

namespace sdl {

class setlocale_t final {
    setlocale_t(){}
    ~setlocale_t(){}
    std::string locale;
    static setlocale_t & instance() {
        static setlocale_t obj;
        return obj;
    }
public:
    static const std::string & get() {
        return instance().locale;
    }
    static void set(const std::string & s) {
        if (s != instance().locale) {
            setlocale(LC_ALL, s.c_str());
            instance().locale = s;
        }
    }
    class auto_locale: noncopyable {
        const std::string old;
    public:
        explicit auto_locale(const std::string & s): old(setlocale_t::get()) {
            setlocale_t::set(s);
        }
        ~auto_locale() {
            setlocale_t::set(old);
        }
    };
};

} // namespace sdl

#endif // __SDL_COMMON_LOCALE_H__