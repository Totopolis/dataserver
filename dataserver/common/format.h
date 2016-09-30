// format.h
//
#pragma once
#ifndef __SDL_COMMON_FORMAT_H__
#define __SDL_COMMON_FORMAT_H__

namespace sdl {

template<int buf_size, typename... Ts> inline
const char * format_s(char(&buf)[buf_size], Ts&&... params) {
    static_assert(buf_size > 20, "");
    if (snprintf(buf, buf_size, std::forward<Ts>(params)...) > 0) {
        buf[buf_size-1] = 0;
        return buf;
    }
    SDL_ASSERT(!"format_s");
    buf[0] = 0;
    return buf;
}

// precision - number of digits after dot; trailing zeros are discarded
template<int buf_size>
const char * format_double(char(&buf)[buf_size], const double value, const int precision) {
    static_assert(buf_size > 20, "");
    static_assert(buf_size > limits::double_max_digits10, "");
    static_assert(limits::double_max_digits10 == 17, "");
#if SDL_DEBUG
    memset_zero(buf);
#endif
    SDL_ASSERT((precision > 0) || debug::unit_test);
    SDL_ASSERT((precision < buf_size - 1) || debug::unit_test);
    int c = snprintf(buf, buf_size, "%#.*f", 
        a_min_max<int, 0, limits::double_max_digits10>(precision),
        value); // print '.' char even if the value is integer
    if (c > 0) { 
        SDL_ASSERT(c < buf_size);
        c = a_min<int, limits::double_max_digits10>(a_min<int, buf_size - 1>(c));
        char * p = buf + c - 1; // pointer to last meaning char 
        while ((p > buf) && (*p == '0')) {
            --p;
        }
        SDL_ASSERT(p >= buf);
        if ((*p == '.') || (*p == ',')) {
            --p;
        }
        SDL_ASSERT((p - buf + 1) >= 0);
        p[1] = 0;
        while(--p >= buf) {
            if (*p == ',') {
                *p = '.';
                break;
            }
        }
        return buf;
    }
    SDL_ASSERT(0);
    buf[0] = 0;
    return buf;
}

} // sdl

#endif // __SDL_COMMON_FORMAT_H__
