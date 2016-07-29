// generator.h
//
#pragma once
#ifndef __SDL_SYSTEM_GENERATOR_H__
#define __SDL_SYSTEM_GENERATOR_H__

#include "system/datatable.h"

namespace sdl { namespace db { namespace make {

class generator : is_static {
    using generator_error = sdl_exception_t<generator>;
public:
    static std::string make_table(database & db, datatable const &);
    static bool make_file(database & db, std::string const & out_file, const char * _namespace = nullptr);
};

struct util : is_static {
    static std::string remove_extension(std::string const& filename);
    static std::string extract_filename(const std::string & path, bool remove_ext);
};

} // make
} // db
} // sdl

#endif // __SDL_SYSTEM_GENERATOR_H__
