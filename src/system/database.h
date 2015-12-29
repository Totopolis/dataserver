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
        sysallocunits = 7,
        sysschobjs = 34,
        syscolpars = 41,
        sysscalartypes = 50,
        sysidxstats = 54,
        sysiscols = 55,
        sysobjvalues = 60,
    };
    enum class sysPage {
        file_header = 0,
        boot_page = 9,
    };
private:
    page_head const * load_sys_obj(sysallocunits const *, sysObj);

    template<class T, sysObj id> 
    std::unique_ptr<T> get_sys_obj(sysallocunits const * p);
    
    template<class T, sysObj id> 
    std::unique_ptr<T> get_sys_obj();

    template<class T, sysObj id> 
    std::vector<std::unique_ptr<T>> get_sys_list();
public:
    explicit database(const std::string & fname);
    ~database();

    const std::string & filename() const;

    bool is_open() const;

    size_t page_count() const;

    page_head const * load_page(pageIndex);
    page_head const * load_page(pageFileID const &);

    std::unique_ptr<bootpage> get_bootpage();
    std::unique_ptr<fileheader> get_fileheader();
    std::unique_ptr<datapage> get_datapage(pageIndex);

    std::unique_ptr<sysallocunits> get_sysallocunits();
    std::unique_ptr<sysschobjs> get_sysschobjs();
    std::unique_ptr<syscolpars> get_syscolpars();
    std::unique_ptr<sysidxstats> get_sysidxstats();
    std::unique_ptr<sysscalartypes> get_sysscalartypes();
    std::unique_ptr<sysobjvalues> get_sysobjvalues();
    std::unique_ptr<sysiscols> get_sysiscols();   

    std::vector<std::unique_ptr<sysschobjs>> get_sysschobjs_list();
public:
    void const * start_address() const; // diagnostic only
    void const * memory_offset(void const * p) const; // diagnostic only
private:
    page_head const * load_page(sysPage);
    page_head const * load_next(page_head const *);
    page_head const * load_prev(page_head const *);
    std::vector<page_head const *> load_page_list(page_head const *);
private:
    class data_t;
    std::unique_ptr<data_t> m_data;
};

} // db
} // sdl

#endif // __SDL_SYSTEM_DATABASE_H__
