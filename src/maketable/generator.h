// generator.h
//
#pragma once
#ifndef __SDL_SYSTEM_GENERATOR_H__
#define __SDL_SYSTEM_GENERATOR_H__

#include "system/datatable.h"

namespace sdl { namespace db { namespace make {

using vector_string = std::vector<std::string>;

class generator : is_static {
    using generator_error = sdl_exception_t<generator>;
public:
    static std::string make_table(database & db, datatable const &);
    static bool make_file(database & db, std::string const & out_file, const char * _namespace = nullptr);
    static bool make_file_exclude(database & db, std::string const & out_file,
        vector_string const & exclude,
        const char * _namespace = nullptr);
};

struct util : is_static {
    static std::string remove_extension(std::string const& filename);
    static std::string extract_filename(const std::string & path, bool remove_ext);
    static vector_string split(const std::string &, char token = ' ');
    static bool is_find(vector_string const &, const std::string &);
};

} // make
} // db
} // sdl

#endif // __SDL_SYSTEM_GENERATOR_H__
