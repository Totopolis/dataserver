// export_database.h
//
#pragma once
#ifndef __SDL_SYSTEM_EXPORT_DATABASE_H__
#define __SDL_SYSTEM_EXPORT_DATABASE_H__

namespace sdl { namespace db { namespace make {

class export_database : is_static {
    using export_database_error = sdl_exception_t<export_database>;
public:
    struct param_type {
        std::string in_file;
        std::string out_file;
        std::string source;
        std::string dest;
        bool empty() const {
            return in_file.empty()
                || out_file.empty()
                || source.empty()
                || dest.empty();
        }
    };
    static bool make_file(param_type const &);
private:
    template<class T> static bool read_input(T &, std::string const & in_file);
    template<class T> static std::string make_script(T const &, param_type const &);
    static bool write_output(std::string const & out_file, std::string const &);
};

} // make
} // db
} // sdl

#endif // __SDL_SYSTEM_EXPORT_DATABASE_H__
