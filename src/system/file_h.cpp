// file_h.cpp
//
#include "stdafx.h"
#include "file_h.h"

namespace sdl {

FileHandler::FileHandler(const char* filename, const char* mode)
    : m_fp(NULL)
{
    if (is_str_valid(filename) && is_str_valid(mode))
    {
        m_fp = fopen(filename, mode);
    }
    else
    {
        SDL_ASSERT(false);
    }
}

void FileHandler::close()
{
    if (NULL != m_fp)
    {
        fclose(m_fp);
        m_fp = NULL;
    }
}

bool FileHandler::open(const char* filename, const char* mode)
{
    close();

    if (is_str_valid(filename) && is_str_valid(mode))
    {
        m_fp = fopen(filename, mode);
    }
    else
    {
        SDL_ASSERT(false);
    }
	return is_open();
}

// size effect: sets current position to the beginning of file
size_t FileHandler::file_size()
{
    size_t size = 0;
    if (m_fp)
    {
        fseek(m_fp, 0, SEEK_END); 
        size = ftell(m_fp);
        fseek(m_fp, 0, SEEK_SET); 
    }
    return size;
}

} //namespace sdl
