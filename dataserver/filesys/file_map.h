// file_map.h
//
#pragma once
#ifndef __SDL_FILESYS_FILE_MAP_H__
#define __SDL_FILESYS_FILE_MAP_H__

#include "dataserver/common/common.h"

namespace sdl {

class FileMapping: noncopyable 
{
    using FileMapping_error = sdl_exception_t<FileMapping>;
public:
    FileMapping();
    ~FileMapping();

    // Create file mapping for read-only. Returns nullptr if error
    void const * CreateMapView(const char* filename);

    // Close file mapping
    void UnmapView();

    bool IsFileMapped() const;

    void const * GetFileView() const;
    
    uint64 GetFileSize() const;

private:
    class data_t;
    std::unique_ptr<data_t> m_data;
};

} // namespace sdl

#endif // __SDL_FILESYS_FILE_MAP_H__