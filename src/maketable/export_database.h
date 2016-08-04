// export_database.h
//
#pragma once
#ifndef __SDL_SYSTEM_EXPORT_DATABASE_H__
#define __SDL_SYSTEM_EXPORT_DATABASE_H__

namespace sdl { namespace db { namespace make {

class export_database : is_static {
public:
    static bool make_file(std::string const & in_file, std::string const & out_file);
};

} // make
} // db
} // sdl

#endif // __SDL_SYSTEM_EXPORT_DATABASE_H__
