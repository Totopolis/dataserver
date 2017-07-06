// page_pool_file.h
//
#pragma once
#ifndef __SDL_SYSTEM_PAGE_POOL_FILE_H__
#define __SDL_SYSTEM_PAGE_POOL_FILE_H__

#include "dataserver/system/page_head.h"

#if defined(SDL_OS_WIN32) //&& (SDL_DEBUG > 1)
#define SDL_TEST_PAGE_POOL  1  // experimental
#else
#define SDL_TEST_PAGE_POOL  0
#endif

#if SDL_TEST_PAGE_POOL

#include <fstream>
#if defined(SDL_OS_WIN32)
#include <windows.h>
#endif

namespace sdl { namespace db { namespace pp {

#if defined(SDL_OS_WIN32)
class PagePoolFile_win32 : noncopyable {
public:
    explicit PagePoolFile_win32(const std::string & fname);
    ~PagePoolFile_win32();
    size_t filesize() const { 
        return m_filesize;
    }
    bool is_open() const {
       return (hFile != INVALID_HANDLE_VALUE);
    }
    void read_all(char * dest)  {
       read(dest, 0, filesize());
    }
    void read(char * dest, size_t offset, size_t size);
private:
    size_t seek_beg(size_t offset);
    size_t seek_end();
private:
    size_t m_filesize = 0;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    SDL_DEBUG_CODE(size_t m_seekpos = 0;)
};

#endif // SDL_OS_WIN32

class PagePoolFile_s : noncopyable {
public:
    explicit PagePoolFile_s(const std::string & fname);
    size_t filesize() const { 
        return m_filesize;
    }
    bool is_open() const {
       return m_file.is_open();
    }
    void read_all(char * dest);
    void read(char * dest, size_t offset, size_t size);
private:
    size_t m_filesize = 0;
    std::ifstream m_file;
};

inline PagePoolFile_s::PagePoolFile_s(const std::string & fname)
    : m_file(fname, std::ifstream::in | std::ifstream::binary) {
    if (m_file.is_open()) {
        m_file.seekg(0, std::ios_base::end);
        m_filesize = m_file.tellg();
        m_file.seekg(0, std::ios_base::beg);
    }
}

inline void PagePoolFile_s::read_all(char * const dest){
    SDL_ASSERT(dest);
    m_file.seekg(0, std::ios_base::beg);
    m_file.read(dest, filesize());
}

inline void PagePoolFile_s::read(char * const dest, const size_t offset, const size_t size) {
    SDL_ASSERT(dest);
    SDL_ASSERT(size && !(size % page_head::page_size));
    SDL_ASSERT(offset + size <= filesize());
    m_file.seekg(offset, std::ios_base::beg);
    m_file.read(dest, size);
}

#if 0 //defined(SDL_OS_WIN32)
using PagePoolFile = PagePoolFile_win32;
#else
using PagePoolFile = PagePoolFile_s; // faster ?
#endif

} // pp
} // db
} // sdl

#endif // SDL_TEST_PAGE_POOL
#endif // __SDL_SYSTEM_PAGE_POOL_FILE_H__
