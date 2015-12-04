// datapage.h
//
#ifndef __SDL_SYSTEM_DATAPAGE_H__
#define __SDL_SYSTEM_DATAPAGE_H__

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
    page_head const * const head;
    bootpage_row const * const row;
    slot_array const slot;
    bootpage(page_head const * p, bootpage_row const * b)
        : head(p), row(b), slot(p) {
        SDL_ASSERT(head);
        SDL_ASSERT(row);
    }
};

class datapage : noncopyable {
public:
    page_head const * const head;
    slot_array const slot;
    explicit datapage(page_head const * h): head(h), slot(h) {
        SDL_ASSERT(head);
    }
};

class sysallocunits : public datapage {
public:
    explicit sysallocunits(page_head const * h) : datapage(h) {}

    size_t size() const; // # of rows
    sysallocunits_row const * operator[](size_t i) const;

    typedef std::pair<sysallocunits_row const *, size_t> find_result;

    template<class fun_type>
    find_result find_if(fun_type fun) const {
        const size_t sz = this->size();
        for (size_t i = 0; i < sz; ++i) {
            if (sysallocunits_row const * p = (*this)[i]) {
                if (fun(p)) {
                    return find_result(p, i);
                }
            }
        }
        return find_result(); // row not found
    }
    find_result find_auid(uint32) const; // find row with auid
};

} // db
} // sdl

#endif // __SDL_SYSTEM_DATAPAGE_H__
