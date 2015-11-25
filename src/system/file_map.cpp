// file_map.cpp
//
#include "common/common.h"
#include "file_map.h"
#include "file_h.h"

#if defined(WIN32)
#include <windows.h>
#endif

namespace sdl {

#if defined(WIN32)

class FileMapping::data_t: noncopyable 
{
    MapView m_pFileView = nullptr;
    size_t m_FileSize = 0;
public:
    data_t(const char* filename, size_t nSize);
    ~data_t();

    MapView GetFileView() const
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
                nullptr,                       // lpSecurityAttributes
                OPEN_EXISTING,              // dwCreationDisposition
                FILE_FLAG_BACKUP_SEMANTICS, // dwFlagsAndAttributes
                nullptr);                      // hTemplateFile
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
};

FileMapping::data_t::data_t(const char* filename, const size_t nSize)
{
    if (!is_str_valid(filename) || !nSize) {
        SDL_WARNING(0);
        return;
    }
    
    ReadFileHandler file(filename);
    if (!file.is_open()) {
        SDL_WARNING(0);
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
        SDL_WARNING(0);
        return;
    }

    m_pFileView = ::MapViewOfFile(
        hFileMapping,
        FILE_MAP_READ,
        0, 0,           // file offset where the view begins
        0);             // mapping extends from the specified offset to the end of the file mapping.

    ::CloseHandle(hFileMapping);

    if (m_pFileView) {
        m_FileSize = nSize; // success
    }
    else {
        SDL_WARNING(0);
    }
}

FileMapping::data_t::~data_t()
{
    if (m_pFileView != nullptr) {
        ::UnmapViewOfFile(m_pFileView);
        m_pFileView = nullptr;
        m_FileSize = 0;
    }
}

#else // FIXME: not implemented

class FileMapping::data_t: noncopyable
{
public:
    data_t(const char* filename, size_t nSize){}

    MapView GetFileView() const
    {
        return nullptr;
    }
    size_t GetFileSize() const
    {
        return 0;
    }
};

#endif // #if defined(WIN32)

FileMapping::FileMapping()
{
}

FileMapping::~FileMapping()
{
}

FileMapping::MapView
FileMapping::GetFileView() const
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

FileMapping::MapView
FileMapping::CreateMapView(const char* filename, const size_t nSize)
{
    UnmapView();

    std::unique_ptr<data_t> p(new data_t(filename, nSize));

    auto ret = p->GetFileView();
    if (ret) {
        SDL_ASSERT(p->GetFileSize() == nSize);
        m_data.swap(p);
    }
    else {
        SDL_WARNING(0);
    }
    return ret;
}

FileMapping::MapView
FileMapping::CreateMapView(const char* filename)
{
    return CreateMapView(filename, FileMapping::GetFileSize(filename));
}

// Returns file size in bytes
// Size effect: sets current position to the beginning of file
size_t FileMapping::GetFileSize(const char* filename)
{
    FileHandler file(filename, "r");
    if (file.is_open()) {
        return file.file_size();
    }
    return 0;
}

} // namespace sdl

