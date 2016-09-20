// time_util.cpp
//
#include "common.h"
#include "time_util.h"

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

} // sdl



