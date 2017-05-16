// export_database.h
//
#pragma once
#ifndef __SDL_SYSTEM_EXPORT_DATABASE_H__
#define __SDL_SYSTEM_EXPORT_DATABASE_H__

#include "dataserver/common/common.h"

namespace sdl { namespace db { namespace make {

class export_database : is_static {
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
};

} // make
} // db
} // sdl

#endif // __SDL_SYSTEM_EXPORT_DATABASE_H__
