// file_h.cpp
//
#include "dataserver/filesys/file_h.h"
#include <errno.h>

namespace sdl {

FileHandler::FileHandler(const char* filename, const char* mode)
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

// warning: failes for large files
// side effect: sets current position to the beginning of file
size_t FileHandler::filesize(const char* filename)
{
    FileHandler f(filename, "rb");
    if (f.is_open())
    {
        if (fseek(f.get(), 0, SEEK_END)) { // If successful, the function returns zero
            SDL_TRACE("fseek(0, SEEK_END) failed: ", filename);
            SDL_TRACE("errno = ", errno);
            SDL_ASSERT(0);
        }
        const size_t size = ftell(f.get());
        if (fseek(f.get(), 0, SEEK_SET)) {
            SDL_TRACE("fseek(0, SEEK_SET) failed: ", filename);
            SDL_TRACE("errno = ", errno);
            SDL_ASSERT(0);
        }
        SDL_WARNING(size);
        return size;
    }
    return 0;
}

} //namespace sdl
