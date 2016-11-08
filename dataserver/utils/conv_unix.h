// conv_unix.h
//
#pragma once
#ifndef __SDL_UTILS_CONV_UNIX_H__
#define __SDL_UTILS_CONV_UNIX_H__

#include "dataserver/common/common.h"

#if defined(SDL_OS_UNIX)

namespace sdl { namespace db { namespace unix {

std::string iconv_cp1251_to_utf8(const std::string &);

} // unix
} // db
} // sdl

#endif // #if defined(SDL_OS_UNIX)
#endif // __SDL_UTILS_CONV_UNIX_H__
