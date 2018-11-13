// file_h.h
//
#pragma once
#ifndef __SDL_FILESYS_FILE_H_H__
#define __SDL_FILESYS_FILE_H_H__

#include "dataserver/common/common.h"

namespace sdl {

class FileHandler : noncopyable {
public:
    typedef FILE * file_handle;
    FileHandler(const char* filename, const char* mode);
    ~FileHandler();

    bool is_open() const { 
        return (m_fp != nullptr);
    }
    file_handle get() const { 
        return m_fp;
    }
    static std::uintmax_t file_size(const char* filename);
private:
    file_handle m_fp;
};

} //namespace sdl

#endif // __SDL_FILESYS_FILE_H_H__
