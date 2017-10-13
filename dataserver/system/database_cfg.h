// database_cfg.h
//
#pragma once
#ifndef __SDL_SYSTEM_DATABASE_CFG_H__
#define __SDL_SYSTEM_DATABASE_CFG_H__

#include "dataserver/common/common.h"

namespace sdl { namespace db {

struct database_cfg {
    enum { default_period = 15 }; // in seconds 
    enum { default_defrag = min_to_sec<5>::value };
    size_t min_memory = 0;
    size_t max_memory = 0;
    size_t pool_period = default_period; // used to decommit free blocks
    size_t pool_defrag = default_defrag; // used to defragment pool memory (= 0 to disable)
    bool use_page_bpool = false;
    database_cfg() = default;
    explicit database_cfg(bool b) noexcept : use_page_bpool(b) {}
    database_cfg(const size_t s1, const size_t s2) noexcept 
        : min_memory(s1), max_memory(s2){
        SDL_ASSERT(min_memory <= max_memory);
        static_assert(!(default_defrag % default_period), "");
    }
};

} // db
} // sdl

#endif // __SDL_SYSTEM_DATABASE_CFG_H__
