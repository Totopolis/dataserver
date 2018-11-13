// file_h.cpp
//
#include "dataserver/filesys/file_h.h"
#include <errno.h>
#include <fstream>

namespace sdl {

FileHandler::FileHandler(const char * const filename, const char * const mode)
    : m_fp(nullptr)
{
    if (is_str_valid(filename) && is_str_valid(mode)) {
        m_fp = fopen(filename, mode);
    }
    SDL_ASSERT(is_open());
}

FileHandler::~FileHandler()
{
    if (m_fp) {
        fclose(m_fp);
        m_fp = nullptr;
    }
}

/*
#error fails for large files (~5GB)
size_t FileHandler::file_size() const
{
    SDL_ASSERT(is_open());
    size_t size = 0;
    if (m_fp) {
        fseek(m_fp, 0, SEEK_END); 
        size = ftell(m_fp);
        fseek(m_fp, 0, SEEK_SET); 
    }
    return size;
}*/

std::uintmax_t FileHandler::file_size(const char * const filename)
{
    if (is_str_valid(filename)) {
        std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
        if (in.is_open()) {
            return in.tellg(); 
        }
        SDL_TRACE_ERROR(filename);
    }
    SDL_ASSERT(0);
    return 0;
}

} //namespace sdl
