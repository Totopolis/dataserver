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
    using vector_string = std::vector<std::string>;
    static std::string make_table(database const & db, datatable const &, bool is_record_count = false);
    static bool make_file(database const & db, std::string const & out_file, const char * _namespace = nullptr);
    static bool make_file_ex(database const & db, std::string const & out_file,
        vector_string const & include,
        vector_string const & exclude,
        const char * _namespace = nullptr,
        const bool is_record_count = false);
    static std::string make_tables(database const & db, 
        vector_string const & include,
        vector_string const & exclude,
        const bool is_record_count = false);
};

} // make
} // db
} // sdl

#endif // __SDL_SYSTEM_GENERATOR_H__
