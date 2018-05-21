// version_t.h
#pragma once
#ifndef __SDL_COMMON_VERSION_TYPE_H__
#define __SDL_COMMON_VERSION_TYPE_H__

#include "dataserver/common/common.h"

namespace sdl {

struct version_t {
    int major = 0;
    int minor = 0;
    int revision = 0;
    bool empty() const {
        return !(major || minor || revision);
    }
    explicit operator bool() const {
        return !empty();
    }
    static version_t parse(const char *);
    static version_t const & current();
    std::string format() const;
};

inline bool operator < (version_t const & x, version_t const & y) {
    if (x.major < y.major) return true;
    if (y.major < x.major) return false;
    if (x.minor < y.minor) return true;
    if (y.minor < x.minor) return false;
    return x.revision < y.revision;
}

inline version_t const & version_t::current() {
    static_assert(count_of(SDL_DATASERVER_VERSION) > 5, "");
    static const version_t obj = version_t::parse(SDL_DATASERVER_VERSION);
    SDL_ASSERT(obj);
    return obj;
}

#if SDL_DEBUG || defined(SDL_TRACE_RELEASE)
inline std::ostream & operator <<(std::ostream & out, version_t const & v) { // for SDL_TRACE
    out << v.format();
    return out;
}
#endif

} // sdl

#endif // __SDL_COMMON_VERSION_TYPE_H__
