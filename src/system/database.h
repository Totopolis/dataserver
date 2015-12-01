// database.h
//
#ifndef __SDL_SYSTEM_DATABASE_H__
#define __SDL_SYSTEM_DATABASE_H__

#pragma once

#include "page_head.h"
#include "boot_page.h"
#include "sysallocunits.h"

namespace sdl { namespace db {

namespace unit {
    struct pageIndex {};
    struct fileIndex {};
}
typedef quantity<unit::pageIndex, uint32> pageIndex;
typedef quantity<unit::fileIndex, uint16> fileIndex;

class bootpage: noncopyable {
public:
    page_head const * const head = nullptr;
    bootpage_row const * const row = nullptr;
    bootpage(page_head const * p, bootpage_row const * b) : head(p), row(b) {
        SDL_ASSERT(head);
        SDL_ASSERT(row);
    }
};

class datapage : noncopyable {
public:
    page_head const * const head = nullptr;
    slot_array const slot;
    explicit datapage(page_head const * h): head(h), slot(h) {
        SDL_ASSERT(head);
    }
};

class sysallocunits : public datapage {
public:
    explicit sysallocunits(page_head const * h) : datapage(h) {}
    size_t size() const;
    sysallocunits_row const * operator[](size_t) const;
};

class database 
{
    A_NONCOPYABLE(database)
public:
    explicit database(const std::string & fname);
    ~database();

    const std::string & filename() const;

    bool is_open() const;

    size_t page_count() const;

    page_head const * load_page(pageIndex);
    
    std::unique_ptr<bootpage> get_bootpage();
    std::unique_ptr<datapage> get_datapage(pageIndex);
    std::unique_ptr<sysallocunits> get_sysallocunits();

private:
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
