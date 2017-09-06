// time_util.cpp
//
#include "dataserver/common/time_util.h"

namespace sdl {

bool time_util::safe_gmtime(struct tm & dest, const time_t value) {
    memset_zero(dest);
#if defined(SDL_OS_WIN32)
    const errno_t err = ::gmtime_s(&dest, &value);
    SDL_ASSERT(!err);
    return !err;
#else
    struct tm * ptm = ::gmtime_r(&value, &dest);
    SDL_ASSERT(ptm);
    return ptm != nullptr;
#endif
}

/* struct tm {
    int tm_sec;   // seconds after the minute - [0, 60] including leap second
    int tm_min;   // minutes after the hour - [0, 59]
    int tm_hour;  // hours since midnight - [0, 23]
    int tm_mday;  // day of the month - [1, 31]
    int tm_mon;   // months since January - [0, 11]
    int tm_year;  // years since 1900
    int tm_wday;  // days since Sunday - [0, 6]
    int tm_yday;  // days since January 1 - [0, 365]
    int tm_isdst; // daylight savings time flag
}; */

namespace {
    template<class T>
    void convert_time_since_epoch(const T & tt, ::tm * utc_tm, ::tm * local_tm) {
        A_STATIC_ASSERT_TYPE(std::time_t, T);
        A_STATIC_ASSERT_TYPE(std::time_t, time_t);
        SDL_ASSERT(utc_tm || local_tm);
        static std::mutex m_mutex;
        std::unique_lock<std::mutex> lock(m_mutex);
        if (utc_tm) {
            * utc_tm = * std::gmtime(&tt);
        }
        if (local_tm) {
            * local_tm = * std::localtime(&tt);
        }
    }
} // namespace

time_state::time_state(now_type && t): now(std::move(t)) {
    convert_time_since_epoch(std::chrono::system_clock::to_time_t(now), &utc_tm, &local_tm);
    current_year = 1900 + utc_tm.tm_year;
}

} // sdl

#if SDL_DEBUG
namespace sdl { namespace {
    class unit_test {
    public:
        unit_test() {
            if (0) {
                time_state state;
                SDL_ASSERT(state.current_year >= 2017);
                SDL_TRACE("time_seconds = ", cast_duration_seconds(state.now));
                SDL_TRACE("time_milliseconds = ", cast_duration_microseconds(state.now));
            }
            if (0) {
                milliseconds_span test;
                milliseconds_span test2(test);
                test.reset();
                SDL_TRACE("now = ", test.now());
                SDL_TRACE("now_reset = ", test2.now_reset());
            }
            if (0) {
                microseconds_span test;
                microseconds_span test2(test);
                test.reset();
                SDL_TRACE("now = ", test.now());
                SDL_TRACE("now_reset = ", test2.now_reset());
            }
        }
    };
    static unit_test s_test;
}
} // sdl
#endif //#if SDL_DEBUG


