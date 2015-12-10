// file_map.cpp
//
#include "common/common.h"
#include "file_map.h"
#include "file_map_detail.h"
#include <fstream>

namespace sdl {

class FileMapping::data_t: noncopyable
{
    typedef file_map_detail::view_of_file view_of_file;

    view_of_file m_pFileView = nullptr;
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
    static uint64 filesize(const char* filename);
};

uint64 FileMapping::data_t::filesize(const char* filename)
{
    if (is_str_valid(filename)) {
        try {
            std::ifstream in(filename, std::ifstream::in | std::ifstream::binary);
            if (in.is_open()) {
                in.seekg(0, std::ios_base::end);
                return in.tellg();
            }
        }
        catch (std::exception & e)
        {
            SDL_TRACE_2("exception = ", e.what());
        }
    }
    SDL_ASSERT(0);
    return 0;
}

FileMapping::data_t::data_t(const char * const filename)
{
    const uint64 fsize = data_t::filesize(filename);
    if (0 == fsize) {
        SDL_TRACE_2("filesize failed : ", filename);
        SDL_ASSERT(false);
        return;
    }
    SDL_TRACE_2("filesize = ", fsize);
    m_pFileView = file_map_detail::map_view_of_file(filename, 0, fsize);
    if (m_pFileView) {
        m_FileSize = fsize; // success
    }
}

FileMapping::data_t::~data_t()
{
    file_map_detail::unmap_view_of_file(m_pFileView, 0, m_FileSize);
}

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

    auto ret = p->GetFileView();
    if (ret) {
        m_data.swap(p);
    }
    return ret;
}

} // namespace sdl


