// file_h.h
//
#ifndef __SDL_SYSTEM_FILE_H_H__
#define __SDL_SYSTEM_FILE_H_H__

#pragma once

namespace sdl {

class FileHandler : noncopyable {
public:
    typedef FILE * file_handle;
public:
    FileHandler(const char* filename, const char* mode);
    ~FileHandler();

    bool is_open() const { 
        return (m_fp != nullptr);
    }
    file_handle get() const { 
        return m_fp;
    }
private:
    // warning: failes for large files
    // side effect: moves current position to the beginning of file
    static size_t filesize(const char* filename);
private:
    file_handle m_fp;
};

} //namespace sdl

#endif // __SDL_SYSTEM_FILE_H_H__
