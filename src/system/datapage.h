// datapage.h
//
#ifndef __SDL_SYSTEM_DATAPAGE_H__
#define __SDL_SYSTEM_DATAPAGE_H__

#pragma once

#include "page_head.h"
#include "sysobj/boot_page.h"
#include "sysobj/file_header.h"
#include "sysobj/sysallocunits.h"
#include "sysobj/sysschobjs.h"
#include "sysobj/syscolpars.h"
#include "sysobj/sysidxstats.h"
#include "sysobj/sysscalartypes.h"
#include "sysobj/sysobjvalues.h"
#include "sysobj/sysiscols.h"
#include "sysobj/sysrowsets.h"
#include "sysobj/pfs_page.h"
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

template<class T>
class datapage_t : noncopyable {
    using datapage_error = sdl_exception_t<datapage_t>;
public:
    using row_type = T;
    using const_pointer = row_type const *;
    using value_type = const_pointer;
    using iterator = slot_iterator<datapage_t>;
public:
    static const char * name();
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
    const_pointer front() const {
        return (*this)[0];
    }
    const_pointer back() const {
        return (*this)[size()-1];
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
    template<class fun_type>
    size_t lower_bound(fun_type less) const;
};

template<class T>
template<class fun_type>
size_t datapage_t<T>::lower_bound(fun_type less) const {
    size_t count = this->size();
    size_t first = 0;
    while (count) {
        const size_t count2 = count / 2;
        const size_t mid = first + count2;
        if (less((*this)[mid], mid)) {
            first = mid + 1;
            count -= count2 + 1;
        }
        else {
            count = count2;
        }
    }
    SDL_ASSERT(first <= this->size());
    return first;
}

class sysallocunits : public datapage_t<sysallocunits_row> {
    typedef datapage_t<sysallocunits_row> base_type;
public:
    explicit sysallocunits(page_head const * h) : base_type(h) {}
    const_pointer find_auid(uint32) const; // find row with auid
};

using datapage = datapage_t<row_head>;
using shared_datapage = std::shared_ptr<datapage>; 

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
