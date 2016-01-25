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
#include "iam_page.h"
#include "slot_iterator.h"

#include <algorithm>

namespace sdl { namespace db {

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
    using const_pointer = row_head const *;
    using value_type = const_pointer;
    using iterator = slot_iterator<datapage>;
public:
    page_head const * const head;
    slot_array const slot;
    explicit datapage(page_head const * h): head(h), slot(h) {
        SDL_ASSERT(head);
    }
    bool empty() const {
        return slot.empty();
    }
    size_t size() const {
        return slot.size();
    }
    const_pointer operator[](size_t i) const;
    iterator begin() const {
        return iterator(this);
    }
    iterator end() const {
        return iterator(this, slot.size());
    }
};

template<class _row_type>
class datapage_t : noncopyable {
public:
    using row_type = _row_type;
    using const_pointer = row_type const *;
    using value_type = const_pointer;
    using iterator = slot_iterator<datapage_t>;
public:
    page_head const * const head;
    slot_array const slot;

    explicit datapage_t(page_head const * h): head(h), slot(h) {
        SDL_ASSERT(head);
        static_assert(sizeof(row_type) < page_head::body_size, "");
    }
    bool empty() const {
        return slot.empty();
    }
    size_t size() const {
        return slot.size();
    }
    const_pointer operator[](size_t i) const {
        return cast::page_row<row_type>(this->head, this->slot[i]);
    }
    iterator begin() const {
        return iterator(this);
    }
    iterator end() const {
        return iterator(this, slot.size());
    }
    template<class fun_type>
    const_pointer find_if(fun_type fun) const {
        auto const last = this->end();
        auto const it = std::find_if(this->begin(), last, fun);
        if (it != last) {
            return *it;
        }
        return nullptr; // row not found
    }
    template<class fun_type>
    void for_row(fun_type fun) const {
        for (auto p : *this) {
            A_STATIC_CHECK_TYPE(const_pointer, p);
            fun(p);
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
    const_pointer find_auid(uint32) const; // find row with auid
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

class iam_page : public datapage_t<iam_page_row> {
    typedef datapage_t<iam_page_row> base_type;
public:
    explicit iam_page(page_head const * h) : base_type(h) {}
};

template<class row_type> inline
std::string col_name_t(row_type const * p) {
    SDL_ASSERT(p);
    using info = typename row_type::info;
    return info::col_name(*p);
}

} // db
} // sdl

#endif // __SDL_SYSTEM_DATAPAGE_H__
