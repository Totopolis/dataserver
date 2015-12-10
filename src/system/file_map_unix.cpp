// file_map_unix.cpp
//
#include "common/common.h"
#include "file_map_detail.h"

#if defined(SDL_OS_UNIX)

#include "file_h.h"
#include <sys/mman.h>

namespace sdl {

file_map_detail::view_of_file 
file_map_detail::map_view_of_file(const char* filename,
                                  uint64 const offset,  
                                  uint64 const size)
{
    static_assert(sizeof(void *) == sizeof(uint64), "64-bit only");

    SDL_ASSERT(size);
    SDL_ASSERT(!offset); // current limitation
    SDL_ASSERT(offset < size);

    if (size && (offset < size) && !offset) {

        FileHandler fp(filename, "rb");
        if (!fp.is_open()) {
            SDL_TRACE_2("FileHandler failed to open file: ", filename);
            SDL_ASSERT(false);
            return nullptr;
        }
        if (size_t(-1) < size) {
            SDL_TRACE_2("file size too large: ", size);
            SDL_ASSERT(false);
            return nullptr;
        }
        auto pFileView = ::mmap(0, 
            static_cast<size_t>(size), 
            PROT_READ, MAP_PRIVATE, fileno(fp.get()), 0);

        if (pFileView == MAP_FAILED) {
            SDL_TRACE_2("mmap failed: ", filename);
            SDL_ASSERT(false);
            return nullptr;
        }
        static_assert(std::is_same<view_of_file, decltype(pFileView)>::value, "");
        return pFileView;
    }
    return nullptr;
}

bool file_map_detail::unmap_view_of_file(
    view_of_file p,
    uint64 const offset,
    uint64 const size)
{
    if (p) {
        SDL_ASSERT(offset < size);
        ::munmap(p, size);
        return true;
    }
    SDL_ASSERT(!offset);
    SDL_ASSERT(!size);
    return false;
}

} // sdl

#endif //#if defined(SDL_OS_UNIX)