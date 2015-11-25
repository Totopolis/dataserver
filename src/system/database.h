// database.h
//
#ifndef __SDL_SYSTEM_DATABASE_H__
#define __SDL_SYSTEM_DATABASE_H__

#pragma once

#include "page_head.h"
#include "boot_page.h"

namespace sdl { namespace db {

class database 
{
	A_NONCOPYABLE(database)
public:
    enum class sys_page
    {
	    file_header = 1,
	    boot_page = 9, // Each database has a single boot page at 1:9
    };
    static pageId_t sys_id(sys_page t)
    {
	    return pageId_t(static_cast<pageId_t::value_type>(t));
    } 
public:
	explicit database(const std::string & fname);
	~database();

	const std::string & filename() const;

	bool is_open() const;

	size_t page_count() const;

	page_header const * load_page(pageId_t);	
    
    page_header const * load_page(sys_page t)
    {
		return load_page(sys_id(t));
	}
	
	bootpage_t bootpage();

	//FIXME: get page slots array

private:
	class data_t;
	std::unique_ptr<data_t> m_data;
};

} // db
} // sdl

#endif // __SDL_SYSTEM_DATABASE_H__
