// file_map_detail.h
//
#ifndef __SDL_SYSTEM_FILE_MAP_DETAIL_H__
#define __SDL_SYSTEM_FILE_MAP_DETAIL_H__

#pragma once

namespace sdl {

struct file_map_detail
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

#endif // __SDL_SYSTEM_FILE_MAP_DETAIL_H__