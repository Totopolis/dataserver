// conv.h
//
#pragma once
#ifndef __SDL_UTILS_CONV_H__
#define __SDL_UTILS_CONV_H__

#include "system/page_type.h"

namespace sdl { namespace db {

struct conv : is_static {
    static std::wstring cp1251_to_wide(std::string const &); // https://en.wikipedia.org/wiki/Windows-1251
    static std::string cp1251_to_utf8(std::string const &);
    static std::wstring utf8_to_wide(std::string const &);
    static std::string wide_to_utf8(std::wstring const &);
    static std::string nchar_to_utf8(vector_mem_range_t const &);
    static std::wstring nchar_to_wide(vector_mem_range_t const &);
    static bool is_utf8(std::string const &);
    static bool method_stop();
    static void method_stop(bool);
};

} // db
} // sdl

#endif // __SDL_UTILS_CONV_H__
