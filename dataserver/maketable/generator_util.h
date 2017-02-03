// generator_util.h
//
#pragma once
#ifndef __SDL_SYSTEM_GENERATOR_UTIL_H__
#define __SDL_SYSTEM_GENERATOR_UTIL_H__

#include "dataserver/common/common.h"

namespace sdl { namespace db { namespace make {

struct util : is_static {
    using vector_string = std::vector<std::string>;
    static std::string remove_extension(std::string const& filename);
    static std::string extract_filename(const std::string & path, bool remove_ext);
    static vector_string split(const std::string &, char token = ' ');
    static bool is_find(vector_string const &, const std::string &);
    static std::string & replace(std::string &, const char * const token, const std::string & value);
    static std::string & replace_double(std::string &, const char * token, double value, int precision);
};

inline std::string & replace(std::string & s, const char * const token, const std::string & value) {
    return util::replace(s, token, value);
};

inline std::string & replace(std::string & s, const char * const token, const char * value) {
    return util::replace(s, token, value);
};

template<typename T>
std::string & replace(std::string & s, const char * const token, const T & value) {
    std::stringstream ss;
    ss << value;
    return util::replace(s, token, ss.str());
};

template<typename T> inline
std::string replace_(const char buf[], const char * const token, const T & value) {
    std::string s(buf);
    replace(s, token, value);
    return s;
};

template<typename T> inline
std::string replace_(std::string && s, const char * const token, const T & value) {
    replace(s, token, value);
    return std::move(s);
};

} // make
} // db
} // sdl

#endif // __SDL_SYSTEM_GENERATOR_UTIL_H__
