// format.cpp
//
#include "dataserver/common/format.h"

namespace sdl {

// precision - number of digits after dot; trailing zeros are discarded
const char * format_detail::format_double(char * const buf, const size_t buf_size,
                                          const double value, const int precision)
{
    SDL_ASSERT(buf_size > 20);
    SDL_ASSERT(buf_size > limits::double_max_digits10);
    static_assert(limits::double_max_digits10 == 17, "");
    if (!buf_size) {
        return "";
    }
#if SDL_DEBUG
    memset(buf, 0, sizeof(buf[0]) * buf_size);
#endif
    SDL_ASSERT((precision > 0) || debug::is_unit_test());
    SDL_ASSERT((precision < buf_size - 1) || debug::is_unit_test());
    int c = snprintf(buf, buf_size, "%#.*f", 
        a_min_max<int, 0, limits::double_max_digits10>(precision),
        value); // print '.' char even if the value is integer
    if (c > 0) { 
        SDL_ASSERT(c < buf_size);
        c = a_min<int, limits::double_max_digits10>(a_min<int>(c, (int)buf_size - 1));
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
