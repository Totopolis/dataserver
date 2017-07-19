// file.cpp
//
#include "dataserver/bpool/file.h"

namespace sdl { namespace db { namespace bpool {

#if defined(SDL_OS_WIN32)

//https://msdn.microsoft.com/en-us/library/windows/desktop/aa364218(v=vs.85).aspx
// The amount of I/O performance improvement that file data caching offers depends on 
// the size of the file data block being read or written. 
// When large blocks of file data are read and written, 
// it is more likely that disk reads and writes will be necessary to finish the I/O operation. 
// I/O performance will be increasingly impaired as more of this kind of I/O operation occurs.
// In these situations, caching can be turned off. 
// This is done at the time the file is opened by passing FILE_FLAG_NO_BUFFERING as a value for the dwFlagsAndAttributes parameter of CreateFile.
// When caching is disabled, all read and write operations directly access the physical disk. 
// However, the file metadata may still be cached.
// To flush the metadata to disk, use the FlushFileBuffers function.

namespace {
#pragma pack(push, 1) 
    union filesize_64 {
        struct lo_hi {
            LONG lo;
            LONG hi;
        } d;
        uint64 size;
        static_assert(4 == sizeof(LONG), "");
        static_assert(sizeof(uint32) == sizeof(LONG), "");
        static_assert(sizeof(lo_hi) == sizeof(uint64), "");
    };
    static_assert(sizeof(filesize_64) == sizeof(uint64), "");
#pragma pack(pop)
} // namespace

PagePoolFile_win32::PagePoolFile_win32(const std::string & fname)
{
#define SDL_OS_WIN32_FILE_NO_BUFFERING  0 
#if SDL_OS_WIN32_FILE_NO_BUFFERING
    SDL_TRACE(__FUNCTION__, " NO_BUFFERING"); // slow
#else
    SDL_TRACE(__FUNCTION__, " WITH BUFFERING");
#endif
    SDL_ASSERT(!fname.empty());
    if (!fname.empty()) {
        hFile = ::CreateFileA(fname.c_str(), // lpFileName
            GENERIC_READ,               // dwDesiredAccess
            FILE_SHARE_READ,            // dwShareMode
            nullptr,                    // lpSecurityAttributes
            OPEN_EXISTING,              // dwCreationDisposition
#if SDL_OS_WIN32_FILE_NO_BUFFERING
            FILE_FLAG_NO_BUFFERING|FILE_ATTRIBUTE_READONLY,
#else
            FILE_ATTRIBUTE_READONLY,    // WITH BUFFERING
#endif
            nullptr);                   // hTemplateFile
        SDL_ASSERT(hFile != INVALID_HANDLE_VALUE);
        if (hFile != INVALID_HANDLE_VALUE) {
            m_filesize = seek_end();
            seek_beg(0);
        }
    }
    throw_error_if_not_t<PagePoolFile_win32>(is_open() && m_filesize,
        "CreateFileA failed");
#undef SDL_OS_WIN32_FILE_NO_BUFFERING
}

PagePoolFile_win32::~PagePoolFile_win32() {
    if (hFile != INVALID_HANDLE_VALUE) {
        ::CloseHandle(hFile);
    }
}

size_t PagePoolFile_win32::seek_beg(const size_t offset)
{
    SDL_ASSERT(offset <= m_filesize);
    filesize_64 fsize {};
    fsize.size = offset;
    fsize.d.lo = ::SetFilePointer(hFile,
        fsize.d.lo,     // lDistanceToMove
        &(fsize.d.hi),  // lpDistanceToMoveHigh
        FILE_BEGIN      // dwMoveMethod
    );
    SDL_DEBUG_CPP(m_seekpos = fsize.size);
    return fsize.size;
}

size_t PagePoolFile_win32::seek_end()
{
    filesize_64 fsize {};
    fsize.d.lo = ::SetFilePointer(hFile,
        fsize.d.lo,     // lDistanceToMove
        &(fsize.d.hi),  // lpDistanceToMoveHigh
        FILE_END        // dwMoveMethod
    );
    SDL_DEBUG_CPP(m_seekpos = fsize.size);
    return fsize.size;
}

void PagePoolFile_win32::read(char * lpBuffer, const size_t offset, size_t size)
{
    SDL_ASSERT(lpBuffer);
    SDL_ASSERT(size && !(size % page_head::page_size));
    SDL_ASSERT(offset + size <= filesize());
    seek_beg(offset);
    SDL_ASSERT(m_seekpos == offset);
    DWORD nNumberOfBytesToRead, outBytesToRead;
    enum { maxBytesToRead = megabyte<16>::value };
    while (size) {
        if (size <= maxBytesToRead) {
            nNumberOfBytesToRead = static_cast<DWORD>(size);
            size = 0;
        }
        else {
            nNumberOfBytesToRead = maxBytesToRead;
            size -= maxBytesToRead;
        }
        outBytesToRead = 0;
        const auto OK = ::ReadFile(hFile,
            lpBuffer,
            nNumberOfBytesToRead,
            &outBytesToRead,        // lpNumberOfBytesRead,
            nullptr);               // lpOverlapped
        SDL_DEBUG_CPP(const auto test = reinterpret_cast<page_head const *>(lpBuffer));
        lpBuffer += nNumberOfBytesToRead;
        SDL_ASSERT(OK);
        SDL_ASSERT(nNumberOfBytesToRead == outBytesToRead);
        SDL_DEBUG_CPP(m_seekpos += outBytesToRead);
#if (SDL_DEBUG > 1)
        throw_error_if_not_t<PagePoolFile_win32>(OK &&
            (nNumberOfBytesToRead == outBytesToRead), "ReadFile failed");
#endif
    }
    SDL_ASSERT(m_seekpos <= filesize());
}

#endif // #if defined(SDL_OS_WIN32)

}}} // sdl