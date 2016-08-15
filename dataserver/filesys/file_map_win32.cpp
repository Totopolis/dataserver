// file_map_win32.cpp
//
#include "common/common.h"
#include "file_map_detail.h"

#if defined(SDL_OS_WIN32)

/*#if !defined(NOMINMAX)
//warning: <windows.h> defines macros min and max which conflict with std::numeric_limits !
#define NOMINMAX
#endif*/

#include <windows.h>

namespace sdl { namespace {

#pragma pack(push, 1) 

union filesize_64 {
    struct {
        uint32 lo;
        uint32 hi;
    } d;
    uint64 size;
};

#pragma pack(pop)

class ReadFileHandler : noncopyable
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
public:
    explicit ReadFileHandler(const char* filename) {
        hFile = ::CreateFileA(filename, // lpFileName
            GENERIC_READ,               // dwDesiredAccess
            FILE_SHARE_READ,            // dwShareMode
            nullptr,                    // lpSecurityAttributes
            OPEN_EXISTING,              // dwCreationDisposition
            FILE_FLAG_BACKUP_SEMANTICS, // dwFlagsAndAttributes
            nullptr);                   // hTemplateFile
    }
    ~ReadFileHandler()
    {
        if (hFile != INVALID_HANDLE_VALUE) {
            ::CloseHandle(hFile);
        }
    }
    HANDLE get() const {
        return hFile;
    }
    bool is_open() const {
        return (hFile != INVALID_HANDLE_VALUE);
    }
};

} // namespace

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

        filesize_64 fsize = {};
        fsize.size = size;

        static_assert(sizeof(DWORD) == 4, "");
        static_assert(sizeof(fsize.size) == 8, "");
        static_assert(sizeof(fsize.d.lo) == 4, "");
        static_assert(sizeof(fsize.d.hi) == 4, "");
        SDL_ASSERT(fsize.d.lo);

        ReadFileHandler file(filename);
        if (!file.is_open()) {
            SDL_TRACE("failed to open file: ", filename);
            SDL_ASSERT(false);
            return nullptr;
        }

        // If the function fails, the return value is NULL
        HANDLE hFileMapping = ::CreateFileMapping(
            file.get(),
            nullptr,
            PAGE_READONLY,
            fsize.d.hi,  // dwMaximumSizeHigh,
            fsize.d.lo,  // dwMaximumSizeLow,
            nullptr);

        if (!hFileMapping) {
            SDL_TRACE("CreateFileMapping failed : ", filename);
            SDL_TRACE("GetLastError = ", GetLastError());
            SDL_ASSERT(false);
            return nullptr;
        }

        auto pFileView = ::MapViewOfFile(
            hFileMapping,
            FILE_MAP_READ,
            0, 0,           // file offset where the view begins
            0);             // mapping extends from the specified offset to the end of the file mapping.

        ::CloseHandle(hFileMapping);

        if (!pFileView) {
            SDL_TRACE("MapViewOfFile failed : ", filename);
            SDL_TRACE("GetLastError = ", GetLastError());
            SDL_TRACE((GetLastError() == ERROR_NOT_ENOUGH_MEMORY) ? "ERROR_NOT_ENOUGH_MEMORY" : "");
            SDL_ASSERT(false);
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
        ::UnmapViewOfFile(p);
        return true;
    }
    SDL_ASSERT(!offset);
    SDL_ASSERT(!size);
    return false;
}

} // sdl

#if SDL_DEBUG
namespace sdl {
    namespace {
        class unit_test {
        public:
            unit_test()
            {
                SDL_TRACE_FILE;
                A_STATIC_ASSERT_IS_POD(filesize_64);
                static_assert(sizeof(filesize_64) == 8, "");
            }
        };
        static unit_test s_test;
    }
} // sdl
#endif //#if SV_DEBUG
#endif //#if defined(SDL_OS_WIN32)