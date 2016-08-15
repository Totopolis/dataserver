// database_fwd.h
//
#pragma once
#ifndef __SDL_SYSTEM_DATABASE_FWD_H__
#define __SDL_SYSTEM_DATABASE_FWD_H__

#include "page_type.h"

namespace sdl { namespace db {

class database;
struct page_head;

struct fwd : is_static { 
    static page_head const * load_page_head(database *, pageFileID const &);
    static page_head const * load_next_head(database *, page_head const *);
    static page_head const * load_prev_head(database *,page_head const *);
    static recordID load_next_record(database *, recordID const &);
    static recordID load_prev_record(database *, recordID const &);
    static pageFileID nextPageID(database *, pageFileID const &);
    static pageFileID prevPageID(database *, pageFileID const &);
};

} // db
} // sdl

#endif // __SDL_SYSTEM_DATABASE_FWD_H__
