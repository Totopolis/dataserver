// file_h.h
//
#ifndef __SDL_SYSTEM_FILE_H_H__
#define __SDL_SYSTEM_FILE_H_H__

#pragma once

namespace sdl {

class FileHandler
{
    A_NONCOPYABLE(FileHandler)
public:
    typedef FILE * file_handle;
    FileHandler() : m_fp(nullptr) {}
    FileHandler(const char* filename, const char* mode);
    ~FileHandler() { close(); }
    bool is_open() const { return (m_fp != nullptr ); }
    file_handle get() const { return m_fp; }
    void close();
    bool open(const char* filename, const char* mode);
    size_t file_size(); // side effect: sets current position to the beginning of file
private:
    file_handle m_fp;
};

} //namespace sdl

#endif // __SDL_SYSTEM_FILE_H_H__
