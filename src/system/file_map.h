// file_map.h
//
#pragma once

namespace sdl {

class FileMapping
{
	A_NONCOPYABLE(FileMapping)
public:
	FileMapping();
	~FileMapping();

	// Returns file size in bytes
	// Size effect: sets current position to the beginning of file
	static size_t GetFileSize(const char* filename);

	typedef void const * MapView;

	// Create file mapping for read-only
	// nSize - size of file mapping in bytes
	// Returns NULL if error
	MapView CreateMapView(const char* filename, size_t nSize);
	MapView CreateMapView(const char* filename);

	// Close file mapping
	void UnmapView();

	bool IsFileMapped() const
	{
		return m_pFileView != NULL;
	}

	MapView GetFileView() const
	{
		return m_pFileView;
	}

	size_t GetFileSize() const 
	{
		return m_FileSize;
	}
private:
	MapView m_pFileView;
	size_t m_FileSize;
};

} // namespace sdl

