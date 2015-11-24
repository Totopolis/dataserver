// database.h
//
#pragma once

#include "page_head.h"
#include "boot_page.h"

namespace sdl { namespace db {

enum class sys_page
{
	file_header = 1,
	boot_page = 9, // Each database has a single boot page at 1:9
};

inline pageId_t sys_id(sys_page t) {
	return pageId_t(static_cast<pageId_t::value_type>(t));
}

class database {
	A_NONCOPYABLE(database)
public:
	explicit database(const std::string & fname);
	~database();

	const std::string & filename() const;

	bool is_open() const;

	size_t page_count() const;

	page_header const * load_page(pageId_t);
	page_header const * load_page(sys_page t) {
		return load_page(sys_id(t));
	}
	std::pair<page_header const *, bootpage_row const *> bootpage();

	//FIXME: get page slots array

private:
	class data_t;
	std::unique_ptr<data_t> m_data;
};

} // db
} // sdl
