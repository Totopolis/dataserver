// generator.h
//
#pragma once
#ifndef __SDL_SYSTEM_GENERATOR_H__
#define __SDL_SYSTEM_GENERATOR_H__

#include "dataserver/system/datatable.h"

namespace sdl { namespace db { namespace make {

class generator : is_static {
    using generator_error = sdl_exception_t<generator>;
public:
    using vector_string = std::vector<std::string>;
    struct param_type {
        std::string out_file;
        std::string schema_names;
        vector_string include;
        vector_string exclude;
        std::string make_namespace;
        bool is_record_count = false;
    };
    static std::string make_table(database const & db, datatable const &, bool is_record_count = false);
    static bool make_file(database const & db, param_type const &);
    static std::string make_tables(database const & db, 
        vector_string const & include,
        vector_string const & exclude,
        bool is_record_count = false);
};

} // make
} // db
} // sdl

#endif // __SDL_SYSTEM_GENERATOR_H__
