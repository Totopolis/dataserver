// file_map.cpp
//
#include "common/common.h"
#include "file_map.h"
#include "file_h.h"
#include <fstream>

#if defined(SDL_OS_WIN32)
/*#if !defined(NOMINMAX)
//warning: <windows.h> defines macros min and max which conflict with std::numeric_limits !
#define NOMINMAX
#endif*/
#include <windows.h>
#elif defined(SDL_OS_UNIX)
#include <sys/mman.h>
#endif

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


#if defined(SDL_OS_WIN32)

class ReadFileHandler : noncopyable
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
public:
    explicit ReadFileHandler(const char* filename) {
        hFile = ::CreateFile(filename,  // lpFileName
            GENERIC_READ,               // dwDesiredAccess
            0,                          // dwShareMode
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
#endif // #if defined(SDL_OS_WIN32)

} // namespace

class FileMapping::data_t: noncopyable
{
    void * m_pFileView = nullptr;
    uint64 m_FileSize = 0;
public:
    explicit data_t(const char* filename);
    ~data_t();

    void const * GetFileView() const
    {
        return m_pFileView;
    }
    uint64 GetFileSize() const
    {
        return m_FileSize;
    }
    static filesize_64 filesize(const char* filename);
};

filesize_64 FileMapping::data_t::filesize(const char* filename)
{
    filesize_64 fsize = {};
    try {
        std::ifstream in(filename, std::ifstream::in | std::ifstream::binary);
        if (in.is_open()) {
            in.seekg(0, std::ios_base::end);
            std::ifstream::pos_type size = in.tellg();
            fsize.size = size;
            return fsize;
        }
    }
    catch (std::exception & e)
    {
        SDL_TRACE_2("exception = ", e.what());
    }
    SDL_ASSERT(0 == fsize.size);
    return fsize;
}

#if defined(SDL_OS_WIN32)
FileMapping::data_t::data_t(const char * const filename)
{
    SDL_ASSERT(is_str_valid(filename));
    SDL_ASSERT(!m_pFileView);
    SDL_ASSERT(!m_FileSize);

    const filesize_64 fsize = data_t::filesize(filename);
    if (0 == fsize.size) {
        SDL_TRACE_2("filesize failed : ", filename);
        SDL_ASSERT(false);
        return;
    }
    SDL_TRACE_2("filesize = ", fsize.size);
    ReadFileHandler file(filename);
    if (!file.is_open()) {
        SDL_TRACE_2("FileMapping failed to open file: ", filename);
        SDL_ASSERT(false);
        return;
    }
    static_assert(sizeof(DWORD) == 4, "");
    static_assert(sizeof(fsize.size) == 8, "");
    static_assert(sizeof(fsize.d.lo) == 4, "");
    static_assert(sizeof(fsize.d.hi) == 4, "");
    SDL_ASSERT(fsize.d.lo);

    // If the function fails, the return value is NULL
    HANDLE hFileMapping = ::CreateFileMapping(
        file.get(),
        nullptr,
        PAGE_READONLY,
        fsize.d.hi,  // dwMaximumSizeHigh,
        fsize.d.lo,  // dwMaximumSizeLow,
        nullptr);

    if (!hFileMapping) {
        SDL_TRACE_2("CreateFileMapping failed : ", filename);
        SDL_TRACE_2("GetLastError = ", GetLastError());
        SDL_ASSERT(false);
        return;
    }

    m_pFileView = ::MapViewOfFile(
        hFileMapping,
        FILE_MAP_READ,
        0, 0,           // file offset where the view begins
        0);             // mapping extends from the specified offset to the end of the file mapping.

    ::CloseHandle(hFileMapping);

    if (!m_pFileView) {
        auto const last = GetLastError();
        SDL_TRACE_2("MapViewOfFile failed : ", filename);
        SDL_TRACE_2("GetLastError = ", last);
        SDL_TRACE((last == ERROR_NOT_ENOUGH_MEMORY) ? "ERROR_NOT_ENOUGH_MEMORY" : "");
        SDL_ASSERT(false);
        return;
    }
    m_FileSize = fsize.size; // success
}

FileMapping::data_t::~data_t()
{
    if (m_pFileView) {
        ::UnmapViewOfFile(m_pFileView);
        m_pFileView = nullptr;
        m_FileSize = 0;
    }
}

#elif defined(SDL_OS_UNIX)

FileMapping::data_t::data_t(const char * const filename)
{
    SDL_ASSERT(is_str_valid(filename));
    SDL_ASSERT(!m_pFileView);
    SDL_ASSERT(!m_FileSize);

    filesize_64 const fsize = data_t::filesize(filename);
    if (0 == fsize.size) {
        SDL_TRACE_2("filesize failed : ", filename);
        SDL_ASSERT(false);
        return;
    }
    FileHandler fp(filename, "rb");
    if (!fp.is_open()) {
        SDL_TRACE_2("FileHandler failed to open file: ", filename);
        SDL_ASSERT(false);
        return;
    }
    if (UINT_MAX < fsize.size) {
        static_assert(UINT_MAX == 0xffffffff, "");
        SDL_TRACE_2("file size too large: ", fsize.size);
        SDL_ASSERT(false);
        return;
    }
    m_pFileView = ::mmap(0, static_cast<size_t>(fsize.size), 
        PROT_READ, MAP_PRIVATE, fileno(fp.get()), 0);
    if (m_pFileView == MAP_FAILED) {
        m_pFileView = nullptr;
        SDL_TRACE_2("mmap failed: ", filename);
        SDL_ASSERT(false);
        return;
    }
    m_FileSize = fsize.size; // success
}

FileMapping::data_t::~data_t()
{
    if (m_pFileView) {
        ::munmap(m_pFileView, m_FileSize);
        m_pFileView = nullptr;
        m_FileSize = 0;
    }
}

#else
#error !defined(SDL_OS_UNIX) && !defined(SDL_OS_WIN32)
#endif

//-------------------------------------------------------------------

FileMapping::FileMapping()
{
}

FileMapping::~FileMapping()
{
}

void const * FileMapping::GetFileView() const
{
    if (m_data.get()) {
        return m_data->GetFileView();
    }
    return nullptr;
}

uint64 FileMapping::GetFileSize() const
{
    if (m_data.get()) {
        return m_data->GetFileSize();
    }
    return 0;
}

bool FileMapping::IsFileMapped() const
{
    return (GetFileView() != nullptr);
}

void FileMapping::UnmapView()
{
    m_data.reset();
}

void const * FileMapping::CreateMapView(const char* filename)
{
    UnmapView();

    std::unique_ptr<data_t> p(new data_t(filename));

    void const * ret = p->GetFileView();
    if (ret) {
        m_data.swap(p);
    }
    else {
        SDL_TRACE_2(__FUNCTION__, " failed");
        SDL_ASSERT(false);
    }
    return ret;
}

} // namespace sdl

#if SDL_DEBUG
namespace sdl {
    namespace {
        class unit_test {
        public:
            unit_test()
            {
                SDL_TRACE(__FILE__);
                A_STATIC_ASSERT_IS_POD(filesize_64);
                static_assert(sizeof(filesize_64) == 8, "");
            }
        };
        static unit_test s_test;
    }
} // sdl
#endif //#if SV_DEBUG
