// time_util.h
//
#pragma once
#ifndef __SDL_COMMON_TIME_UTIL_H__
#define __SDL_COMMON_TIME_UTIL_H__

#include "dataserver/common/common.h"
#include <time.h>  /* time_t, struct tm, time, localtime, strftime */

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

inline void time_span::reset() {
    ::time(&m_start);
}

inline time_t time_span::now() const {
    time_t value;
    ::time(&value);
    return (value - m_start);
}

inline time_t time_span::now_reset() {
    time_t res = now();
    reset();
    return res;
}

struct time_util: is_static {
    static bool safe_gmtime(struct tm & dest, time_t);
};

class time_state {
public:
    using now_type = decltype(std::chrono::system_clock::now());
    now_type now;
    ::tm utc_tm;
    ::tm local_tm;
    int current_year = 0;
    explicit time_state(now_type && t);
    time_state(): time_state(std::chrono::system_clock::now()) {}
};

template<class duration_type, class T>
inline long_long cast_duration(const T & now) {
    A_STATIC_ASSERT_TYPE(time_state::now_type, T);
    return std::chrono::duration_cast<duration_type>(
        now.time_since_epoch()).count();
}

template<class T>
inline long_long time_seconds(const T & now) {
    return cast_duration<std::chrono::seconds>(now);
}

template<class T>
inline long_long time_milliseconds(const T & now) {
    return cast_duration<std::chrono::milliseconds>(now) % 1000;
}

template<class T>
inline long_long time_microseconds(const T & now) {
    return cast_duration<std::chrono::microseconds>(now) % 1000000;
}

class microseconds_span {
    using now_type = time_state::now_type;
    now_type m_start;
public:
    microseconds_span(): m_start(std::chrono::system_clock::now()){}
    explicit microseconds_span(now_type const & t): m_start(t) {}
    void reset() {
        m_start = std::chrono::system_clock::now();
    }
    long_long start() const { 
        return time_microseconds(m_start);
    }
    long_long now() const { // microseconds elapsed since reset()
        const auto value = std::chrono::system_clock::now();
        return time_microseconds(value) - start();
    }
    long_long now_reset() { // now() and reset()
        const auto value = std::chrono::system_clock::now();
        const auto res = time_microseconds(value) - start();
        m_start = value;
        return res;
    }
};

} // sdl

#endif // __SDL_COMMON_TIME_UTIL_H__