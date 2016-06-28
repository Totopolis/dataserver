// time_span.h
#pragma once
#ifndef __SDL_COMMON_TIME_SPAN_H__
#define __SDL_COMMON_TIME_SPAN_H__

#include <time.h>

namespace sdl {

class time_span {
    time_t m_start;
public:
    time_span() { reset(); }
    explicit time_span(time_t t): m_start(t) {}
    void reset();
    time_t start() const { return m_start; }
    time_t now() const; // time in seconds elapsed since reset()
    time_t now_reset(); // now() and reset()
};

inline void time_span::reset()
{
    ::time(&m_start);
}

inline time_t time_span::now() const
{
    time_t value;
    ::time(&value);
    return (value - m_start);
}

inline time_t time_span::now_reset()
{
    time_t res = now();
    reset();
    return res;
}

} // namespace sdl

#endif // __SDL_COMMON_TIME_SPAN_H__