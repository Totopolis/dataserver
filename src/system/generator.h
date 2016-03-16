// generator.h
//
#ifndef __SDL_SYSTEM_GENERATOR_H__
#define __SDL_SYSTEM_GENERATOR_H__

#pragma once

#include "datatable.h"

namespace sdl { namespace db { namespace make {

class generator : is_static {
public:
    static std::string make(database & db, datatable const &);
};

} // make
} // db
} // sdl

#endif // __SDL_SYSTEM_GENERATOR_H__
