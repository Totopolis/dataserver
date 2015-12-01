// database.h
//
#ifndef __SDL_SYSTEM_DATABASE_H__
#define __SDL_SYSTEM_DATABASE_H__

#pragma once

#include "page_head.h"
#include "boot_page.h"

namespace sdl { namespace db {

namespace unit {
    struct pageIndex {};
    struct fileIndex {};
}
typedef quantity<unit::pageIndex, uint32> pageIndex;
typedef quantity<unit::fileIndex, uint16> fileIndex;

struct bootpage {
    page_head const * const head;
    bootpage_row const * const row;
    bootpage() : head(nullptr), row(nullptr){}
    bootpage(page_head const * p, bootpage_row const * b) : head(p), row(b){}
};

struct datapage {
    page_head const * const head;
    slot_array slot;
    datapage() : head(nullptr), slot(nullptr) {}
    explicit datapage(page_head const * h): head(h), slot(h) {}
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
    
    bootpage load_bootpage();
    datapage load_datapage(pageIndex);

private:
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
