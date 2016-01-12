// datapage.h
//
#ifndef __SDL_SYSTEM_DATAPAGE_H__
#define __SDL_SYSTEM_DATAPAGE_H__

#pragma once

#include "page_head.h"
#include "boot_page.h"
#include "file_header.h"

#include "sysallocunits.h"
#include "sysschobjs.h"
#include "syscolpars.h"
#include "sysidxstats.h"
#include "sysscalartypes.h"
#include "sysobjvalues.h"
#include "sysiscols.h"

namespace sdl { namespace db {

namespace unit {
    struct pageIndex {};
    struct fileIndex {};
}
typedef quantity<unit::pageIndex, uint32> pageIndex;
typedef quantity<unit::fileIndex, uint16> fileIndex;

inline pageIndex make_page(size_t i) {
    SDL_ASSERT(i < pageIndex::value_type(-1));
    static_assert(pageIndex::value_type(-1) == 4294967295, "");
    return pageIndex(static_cast<pageIndex::value_type>(i));
}

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
    row_head const * get_row_head(size_t) const;
};

template<class row_type>
class datapage_t : noncopyable {
public:
    typedef row_type value_type;
public:
    page_head const * const head;
    slot_array const slot;
    explicit datapage_t(page_head const * h): head(h), slot(h) {
        SDL_ASSERT(head);
        static_assert(sizeof(row_type) < page_head::body_size, "");
    }
    row_type const * operator[](size_t i) const {
        return cast::page_row<row_type>(this->head, this->slot[i]);
    }

    typedef std::pair<row_type const *, size_t> find_result;

    template<class fun_type>
    find_result find_if(fun_type fun) const {
        const size_t sz = slot.size();
        for (size_t i = 0; i < sz; ++i) {
            if (row_type const * p = (*this)[i]) {
                if (fun(p)) {
                    return find_result(p, i);
                }
            }
        }
        return find_result(); // row not found
    }

    template<class fun_type>
    void for_row(fun_type fun) const {
        const size_t sz = slot.size();
        for (size_t i = 0; i < sz; ++i) {
            if (row_type const * p = (*this)[i]) {
                fun(p);
            }
        }
    }
};

class fileheader : public datapage_t<fileheader_row> {
    typedef datapage_t<fileheader_row> base_type;
public:
    explicit fileheader(page_head const * h) : base_type(h) {}
};

class sysallocunits : public datapage_t<sysallocunits_row> {
    typedef datapage_t<sysallocunits_row> base_type;
public:
    explicit sysallocunits(page_head const * h) : base_type(h) {}
    find_result find_auid(uint32) const; // find row with auid
};

class sysschobjs : public datapage_t<sysschobjs_row> {
    typedef datapage_t<sysschobjs_row> base_type;
public:
    explicit sysschobjs(page_head const * h) : base_type(h) {}
};

class syscolpars : public datapage_t<syscolpars_row> {
    typedef datapage_t<syscolpars_row> base_type;
public:
    explicit syscolpars(page_head const * h) : base_type(h) {}
};

class sysidxstats : public datapage_t<sysidxstats_row> {
    typedef datapage_t<sysidxstats_row> base_type;
public:
    explicit sysidxstats(page_head const * h) : base_type(h) {}
};

class sysscalartypes : public datapage_t<sysscalartypes_row> {
    typedef datapage_t<sysscalartypes_row> base_type;
public:
    explicit sysscalartypes(page_head const * h) : base_type(h) {}
};

class sysobjvalues : public datapage_t<sysobjvalues_row> {
    typedef datapage_t<sysobjvalues_row> base_type;
public:
    explicit sysobjvalues(page_head const * h) : base_type(h) {}
};

class sysiscols : public datapage_t<sysiscols_row> {
    typedef datapage_t<sysiscols_row> base_type;
public:
    explicit sysiscols(page_head const * h) : base_type(h) {}
};

} // db
} // sdl

#endif // __SDL_SYSTEM_DATAPAGE_H__
