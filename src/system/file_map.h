// file_map.h
//
#ifndef __SDL_SYSTEM_FILE_MAP_H__
#define __SDL_SYSTEM_FILE_MAP_H__

#pragma once

namespace sdl {

class FileMapping: noncopyable
{
    class FileMapping_error : public sdl_exception {
    public:
        explicit FileMapping_error(const char* s) : sdl_exception(s){}
    };
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

#endif // __SDL_SYSTEM_FILE_MAP_H__