// database.h
//
#ifndef __SDL_SYSTEM_DATABASE_H__
#define __SDL_SYSTEM_DATABASE_H__

#pragma once

#include "datapage.h"

namespace sdl { namespace db {

class database : noncopyable
{
public:
    enum class sysObj {
        sysallocunits_obj = 7,
        syschobjs_obj = 34,
        syscolpars_obj = 41,
        sysscalartypes_obj = 50,
        sysidxstats_obj = 54,
        sysiscols_obj = 55,
        sysobjvalues_obj = 60,
    };
    enum class sysPage {
        file_header = 0,
        boot_page = 9,
    };
public:
    explicit database(const std::string & fname);
    ~database();

    const std::string & filename() const;

    bool is_open() const;

    size_t page_count() const;

    page_head const * load_page(pageIndex);
    page_head const * load_page(pageFileID const &);
    
    std::unique_ptr<bootpage> get_bootpage();
    std::unique_ptr<datapage> get_datapage(pageIndex);
    std::unique_ptr<sysallocunits> get_sysallocunits();
    std::unique_ptr<syschobjs> get_syschobjs();
    std::unique_ptr<syschobjs> get_syschobjs(sysallocunits const *);
    std::unique_ptr<syscolpars> get_syscolpars();
    std::unique_ptr<syscolpars> get_syscolpars(sysallocunits const *);

    void const * start_address() const; // diagnostic only
    void const * memory_offset(void const * p) const; // diagnostic only

private:
    page_head const * load_page(sysPage);
    page_head const * load_next(page_head const *);
    page_head const * load_prev(page_head const *);
private:
    class data_t;
    std::unique_ptr<data_t> m_data;
};

} // db
} // sdl

#endif // __SDL_SYSTEM_DATABASE_H__
