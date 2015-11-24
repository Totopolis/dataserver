// database.cpp
//
#include "stdafx.h"
#include "database.h"
#include "file_map.h"

namespace sdl { namespace db {

class database::data_t : noncopyable
{
	enum { page_size = page_header::page_size };
public:
	const std::string filename;
	explicit data_t(const std::string & fname);

	bool is_open() const
	{
		return m_fmap.IsFileMapped();
	}
	size_t page_count() const
	{	
		return m_pageCount;
	}
	page_header const * load_page(pageId_t i) const;
private:
	size_t m_pageCount;
	FileMapping m_fmap;
};

database::data_t::data_t(const std::string & fname)
	: filename(fname)
	, m_pageCount(0)
{
	A_STATIC_ASSERT(page_size == 8 * 1024);
	if (m_fmap.CreateMapView(fname.c_str())) {
		const size_t sz = m_fmap.GetFileSize();
		SDL_ASSERT(!(sz % page_size));
		m_pageCount = sz / page_size;
	}
}

page_header const *
database::data_t::load_page(pageId_t const i) const
{
	const size_t pageIndex = i.value();
	if (pageIndex < m_pageCount) {
		const char * const data = static_cast<const char *>(m_fmap.GetFileView());
		const char * p = data + pageIndex * page_size;
		return reinterpret_cast<page_header const *>(p);
	}
	SDL_ASSERT(0);
	return nullptr;
}

database::database(const std::string & fname)
	: m_data(new data_t(fname))
{
}

database::~database()
{
}

const std::string & database::filename() const
{
	return m_data->filename;
}

bool database::is_open() const
{
	return m_data->is_open();
}

size_t database::page_count() const
{
	return m_data->page_count();
}

page_header const *
database::load_page(pageId_t i)
{
	return m_data->load_page(i);
}

std::pair<page_header const *, bootpage_row const *>
database::bootpage()
{
	typedef std::pair<page_header const *, bootpage_row const *> bootpage_t;
	if (page_header const * const page = load_page(sys_page::boot_page)) {
		return bootpage_t(page, page_body<bootpage_row>(page));
	}
	return bootpage_t();
}

} // db
} // sdl

