// database_cfg.h
//
#pragma once
#ifndef __SDL_SYSTEM_DATABASE_CFG_H__
#define __SDL_SYSTEM_DATABASE_CFG_H__

#include "dataserver/common/common.h"

namespace sdl { namespace db {

struct database_cfg {
    enum { default_period = 15 };
    size_t min_memory;
    size_t max_memory;
    int pool_period = default_period; // seconds
    bool use_page_bpool = true; // to be tested
    database_cfg() noexcept : min_memory(0), max_memory(0){}
    database_cfg(const size_t s1, const size_t s2) noexcept 
        : min_memory(s1), max_memory(s2){
        SDL_ASSERT(min_memory <= max_memory);
    }
};

} // db
} // sdl

#endif // __SDL_SYSTEM_DATABASE_CFG_H__
