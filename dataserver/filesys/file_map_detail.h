// file_map_detail.h
//
#pragma once
#ifndef __SDL_FILESYS_FILE_MAP_DETAIL_H__
#define __SDL_FILESYS_FILE_MAP_DETAIL_H__

#include "dataserver/common/common.h"

namespace sdl {

struct file_map_detail : is_static
{
    typedef void * view_of_file;
    
    static view_of_file map_view_of_file(
        const char* filename,
        uint64 offset,
        uint64 size);

    static bool unmap_view_of_file(view_of_file, 
        uint64 offset,
        uint64 size);    
};

} // sdl

#endif // __SDL_FILESYS_FILE_MAP_DETAIL_H__