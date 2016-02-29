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
#include "sysrowsets.h"
#include "pfs_page.h"
#include "index_page.h"
#include "slot_iterator.h"
#include "page_iterator.h"

#include <algorithm>
#include <functional>

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

class pfs_page: noncopyable {
    enum { pfs_size = pfs_page_row::pfs_size }; // = 8088
    bool is_pfs(pageFileID const &) const;
public:
    static const char * name() { return "pfs_page"; }
    page_head const * const head;
    pfs_page_row const * const row;
    explicit pfs_page(page_head const *);
    static pageFileID pfs_for_page(pageFileID const &);
    pfs_byte operator[](pageFileID const &) const;
};

class datapage : noncopyable {
    using datapage_error = sdl_exception_t<datapage>;
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
    row_head const & at(size_t) const; // throw_error if empty
};

template<class _row_type>
class datapage_t : noncopyable {
    using datapage_error = sdl_exception_t<datapage_t>;
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
    virtual ~datapage_t(){}

    bool empty() const {
        return slot.empty();
    }
    size_t size() const {
        return slot.size();
    }
    const_pointer operator[](size_t i) const {
        return cast::page_row<row_type>(this->head, this->slot[i]);
    }
    row_type const & at(size_t i) const { // throw_error if empty
        throw_error_if<datapage_error>(i >= size(), "row not found");
        return *(*this)[i];
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
    static const char * name();
};

class sysallocunits : public datapage_t<sysallocunits_row> {
    typedef datapage_t<sysallocunits_row> base_type;
public:
    explicit sysallocunits(page_head const * h) : base_type(h) {}
    const_pointer find_auid(uint32) const; // find row with auid
};

using fileheader = datapage_t<fileheader_row>;
using sysschobjs = datapage_t<sysschobjs_row>;
using syscolpars = datapage_t<syscolpars_row>;
using sysidxstats = datapage_t<sysidxstats_row>;
using sysscalartypes = datapage_t<sysscalartypes_row>;
using sysobjvalues = datapage_t<sysobjvalues_row>;
using sysiscols = datapage_t<sysiscols_row>;
using sysrowsets = datapage_t<sysrowsets_row>;

} // db
} // sdl

#endif // __SDL_SYSTEM_DATAPAGE_H__
