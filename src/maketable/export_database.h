// export_database.h
//
#pragma once
#ifndef __SDL_SYSTEM_EXPORT_DATABASE_H__
#define __SDL_SYSTEM_EXPORT_DATABASE_H__

namespace sdl { namespace db { namespace make {

class export_database : is_static {
    using export_database_error = sdl_exception_t<export_database>;
public:
    using table_source_dest = std::pair<std::string, std::string>;
    static bool make_file(std::string const & in_file, std::string const & out_file, table_source_dest const &);
private:
    template<class T> static bool read_input(T &, std::string const & in_file);
    template<class T> static std::string make_script(T const &, table_source_dest const &);
    static bool write_output(std::string const & out_file, std::string const & script);
};

} // make
} // db
} // sdl

#endif // __SDL_SYSTEM_EXPORT_DATABASE_H__
