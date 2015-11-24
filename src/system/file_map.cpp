// file_map.cpp
//
#include "stdafx.h"
#include "file_map.h"
#include "file_h.h"
#include <windows.h>

namespace sdl {
	
FileMapping::FileMapping()
	: m_pFileView(NULL)
	, m_FileSize(0)
{
}

FileMapping::~FileMapping()
{
	UnmapView();
}

void FileMapping::UnmapView()
{
	if (m_pFileView != NULL) {
		::UnmapViewOfFile(m_pFileView);
		m_pFileView = NULL;
		m_FileSize = 0;
	}
}

FileMapping::MapView
FileMapping::CreateMapView(const char* filename, const size_t nSize)
{
	class ReadFileHandler: noncopyable
	{
		HANDLE hFile;
	public:
		ReadFileHandler(const char* filename) {
			hFile = ::CreateFile(filename,	// lpFileName
				GENERIC_READ,               // dwDesiredAccess
				0,                          // dwShareMode
				NULL,                       // lpSecurityAttributes
				OPEN_EXISTING,              // dwCreationDisposition
				FILE_FLAG_BACKUP_SEMANTICS, // dwFlagsAndAttributes
				NULL);						// hTemplateFile
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

	if (!is_str_valid(filename) || !nSize) {
		SDL_TRACE_2(__FUNCTION__, " failed");
		return NULL;
	}

	UnmapView();

	ReadFileHandler file(filename);
	if (!file.is_open()) {
		return NULL;
	}

	A_STATIC_ASSERT(sizeof(nSize) == sizeof(DWORD));
	const DWORD dwMaximumSizeHigh = 0;
	const DWORD dwMaximumSizeLow = nSize;

	HANDLE hFileMapping = ::CreateFileMapping(
		file.get(),
		NULL,
		PAGE_READONLY,
		dwMaximumSizeHigh,
		dwMaximumSizeLow,
		NULL);

	if (hFileMapping == NULL)
		return NULL;

	m_pFileView = ::MapViewOfFile(
		hFileMapping, 
		FILE_MAP_READ,
		0, 0,			// file offset where the view begins
		0);				// mapping extends from the specified offset to the end of the file mapping.

	::CloseHandle(hFileMapping);

	if (m_pFileView)
		m_FileSize = nSize;

	return m_pFileView;
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

