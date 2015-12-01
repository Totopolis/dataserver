// file_map.cpp
//
#include "common/common.h"
#include "file_map.h"
#include "file_h.h"

#if defined(SDL_OS_WIN32)
#include <windows.h>
#elif defined(SDL_OS_UNIX)
#include <sys/mman.h>
#endif

namespace sdl {

#if defined(SDL_OS_WIN32)

class FileMapping::data_t: noncopyable
{
public:
    explicit data_t(const char* filename);
    ~data_t();

    void const * GetFileView() const
    {
        return m_pFileView;
    }
    size_t GetFileSize() const
    {
        return m_FileSize;
    }
private:
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
private:
    void * m_pFileView = nullptr;
    size_t m_FileSize = 0;

};

FileMapping::data_t::data_t(const char* filename)
{
    SDL_ASSERT(is_str_valid(filename));
    SDL_ASSERT(!m_pFileView);
    SDL_ASSERT(!m_FileSize);

    const size_t nSize = FileHandler::file_size(filename);
    if (!nSize) {
        SDL_ASSERT(false);
        return;
    }
    ReadFileHandler file(filename);
    if (!file.is_open()) {
        SDL_ASSERT(false);
        return;
    }
    static_assert(sizeof(nSize) == sizeof(DWORD), "");
    const DWORD dwMaximumSizeHigh = 0;
    const DWORD dwMaximumSizeLow = nSize;

    // If the function fails, the return value is NULL
    HANDLE hFileMapping = ::CreateFileMapping(
        file.get(),
        nullptr,
        PAGE_READONLY,
        dwMaximumSizeHigh,
        dwMaximumSizeLow,
        nullptr);

    if (!hFileMapping) {
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
        SDL_ASSERT(false);
        return;
    }
    m_FileSize = nSize; // success
    //SDL_TRACE_3(__FUNCTION__, " success: ", m_FileSize);
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

class FileMapping::data_t: noncopyable
{
public:
    explicit data_t(const char* filename);
    ~data_t();

    void const * GetFileView() const
    {
        return m_pFileView;
    }
    size_t GetFileSize() const
    {
        return m_FileSize;
    }
private:
    void * m_pFileView = nullptr;
    size_t m_FileSize = 0; 
};

FileMapping::data_t::data_t(const char* filename)
{
    SDL_TRACE(__FUNCTION__);
    SDL_ASSERT(is_str_valid(filename));
    SDL_ASSERT(!m_pFileView);
    SDL_ASSERT(!m_FileSize);

    FileHandler fp(filename, "rb");
    if (!fp.is_open()) {
        SDL_ASSERT(false);
        return;
    }
    const size_t nSize = fp.file_size();
    if (!nSize) {
        SDL_ASSERT(false);
        return;
    }
    m_pFileView = ::mmap(0, nSize, PROT_READ, MAP_PRIVATE, fileno(fp.get()), 0);
    if (m_pFileView == MAP_FAILED) {
        m_pFileView = nullptr;
        SDL_ASSERT(false);
        return;
    }
    m_FileSize = nSize; // success
    //SDL_TRACE_3(__FUNCTION__, " success: ", m_FileSize);
}

FileMapping::data_t::~data_t()
{
    SDL_TRACE(__FUNCTION__);
    if (m_pFileView) {
        ::munmap(m_pFileView, m_FileSize);
        m_pFileView = nullptr;
        m_FileSize = 0;
    }
}

#else
#error not implemented
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

size_t FileMapping::GetFileSize() const
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

