// file_map_unix.cpp
//
#if defined(SDL_OS_UNIX)

#include "dataserver/filesys/file_map_detail.h"
#include "dataserver/filesys/file_h.h"
#include "dataserver/memory/mmap64_unix.hpp"

namespace sdl {

file_map_detail::view_of_file 
file_map_detail::map_view_of_file(const char* filename,
                                  uint64 const offset,  
                                  uint64 const size)
{
    A_STATIC_ASSERT_64_BIT; 

    SDL_ASSERT(size);
    SDL_ASSERT(!offset); // current limitation
    SDL_ASSERT(offset < size);

    if (size && (offset < size) && !offset) {

        FileHandler fp(filename, "rb");
        if (!fp.is_open()) {
            SDL_TRACE("FileHandler failed to open file: ", filename);
            SDL_ASSERT(false);
            return nullptr;
        }
        auto pFileView = mmap64_t::call(
            nullptr, static_cast<size_t>(size), 
            PROT_READ, MAP_PRIVATE, fileno(fp.get()), 0);

        if (pFileView == MAP_FAILED) {
            SDL_TRACE("mmap failed: ", filename);
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
        if (::munmap(p, size)) {
            SDL_ASSERT(!"munmap");
        }
        return true;
    }
    SDL_ASSERT(!offset);
    SDL_ASSERT(!size);
    return false;
}

} // sdl

#if SDL_DEBUG
namespace sdl { namespace {

class unit_test {
public:
    unit_test()
    {
        SDL_TRACE_FILE;
        //SDL_TRACE("has_mmap64::value = ", has_mmap64::value);
    }
};
static unit_test s_test;

} // namespace
} // sdl
#endif //#if SDL_DEBUG
#endif //#if defined(SDL_OS_UNIX)