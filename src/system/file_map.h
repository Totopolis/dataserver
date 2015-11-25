// file_map.h
//
#ifndef __SDL_SYSTEM_FILE_MAP_H__
#define __SDL_SYSTEM_FILE_MAP_H__

#pragma once

namespace sdl {

class FileMapping
{
    A_NONCOPYABLE(FileMapping)
public:
    typedef void const * MapView;
public:
    FileMapping();
    ~FileMapping();

    // Returns file size in bytes
    // Size effect: sets current position to the beginning of file
    static size_t GetFileSize(const char* filename);

    // Create file mapping for read-only
    // nSize - size of file mapping in bytes
    // Returns NULL if error
    MapView CreateMapView(const char* filename, size_t nSize);
    MapView CreateMapView(const char* filename);

    // Close file mapping
    void UnmapView();

    bool IsFileMapped() const;

    MapView GetFileView() const;
    
    size_t GetFileSize() const;

private:
    class data_t;
    std::unique_ptr<data_t> m_data;
};

} // namespace sdl

#endif // __SDL_SYSTEM_FILE_MAP_H__