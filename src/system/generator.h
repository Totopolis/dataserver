// generator.h
//
#ifndef __SDL_SYSTEM_GENERATOR_H__
#define __SDL_SYSTEM_GENERATOR_H__

#pragma once

#include "datatable.h"

namespace sdl { namespace db { namespace make {

class generator : is_static {
    using generator_error = sdl_exception_t<generator>;
public:
    static std::string make_table(database & db, datatable const &);
    static bool make_file(database & db, std::string const & out_file);
};

} // make
} // db
} // sdl

#endif // __SDL_SYSTEM_GENERATOR_H__
