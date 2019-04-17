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

template<class duration_type>
inline long_long cast_duration(const time_state::now_type & now) {
    return std::chrono::duration_cast<duration_type>(
        now.time_since_epoch()).count();
}

template<class T>
inline long_long cast_duration_seconds(const T & now) {
    return cast_duration<std::chrono::seconds>(now);
}

template<class T>
inline long_long cast_duration_milliseconds(const T & now) {
    return cast_duration<std::chrono::milliseconds>(now);
}

template<class T>
inline long_long cast_duration_microseconds(const T & now) {
    return cast_duration<std::chrono::microseconds>(now);
}

template<class duration_type>
class duration_span {
    using now_type = time_state::now_type;
    now_type m_start;
public:
    using type = duration_type;
    duration_span(): m_start(std::chrono::system_clock::now()){}
    explicit duration_span(now_type const & t): m_start(t) {}
    void reset() {
        m_start = std::chrono::system_clock::now();
    }
    long_long start() const { 
        return cast_duration<duration_type>(m_start);
    }
    long_long now() const { // time elapsed since reset()
        const auto res = cast_duration<duration_type>(std::chrono::system_clock::now()) - start();
        SDL_ASSERT(res >= 0);
        return res;
    }
    long_long now_reset() { // now() and reset()
        const auto value = std::chrono::system_clock::now();
        const auto res = cast_duration<duration_type>(value) - start();
        SDL_ASSERT(res >= 0);
        m_start = value;
        return res;
    }
};

using milliseconds_span = duration_span<std::chrono::milliseconds>;
using microseconds_span = duration_span<std::chrono::microseconds>;

#if SDL_TRACE_ENABLED
class trace_milliseconds_span {
    const char * const name;
    milliseconds_span data;
public:
    explicit trace_milliseconds_span(const char * s): name(s ? s : "") {}
    ~trace_milliseconds_span() {
        const auto t = data.now();
        if (t > 0) {
            SDL_TRACE(name, " = ", t, " ms");
        }
    }
};
#define SDL_MEASURE_LINENAME_CAT(name, line) name##line
#define SDL_MEASURE_LINENAME(name, line) SDL_MEASURE_LINENAME_CAT(name, line)
#define SDL_MEASURE_MILLISECOND     sdl::trace_milliseconds_span SDL_MEASURE_LINENAME(_sdl_, __LINE__)(__func__)
#else
#define SDL_MEASURE_MILLISECOND     ((void)0)
#endif

} // sdl

#endif // __SDL_COMMON_TIME_UTIL_H__