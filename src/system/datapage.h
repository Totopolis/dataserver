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

#include <algorithm>
#include <iterator>

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

template<class T>
class slot_iterator : 
    public std::iterator<
                std::bidirectional_iterator_tag,
                typename T::value_type>
{
public:
    using value_type = typename T::value_type;
private:
    T const * parent;
    size_t slot_index;

    size_t parent_size() const {
        SDL_ASSERT(parent);
        return parent->size();
    }
    
    friend T;
    explicit slot_iterator(T const * p, size_t i = 0)
        : parent(p)
        , slot_index(i)
    {
        SDL_ASSERT(parent);
        SDL_ASSERT(slot_index <= parent->size());
    }
public:
    slot_iterator(): parent(nullptr), slot_index(0) {}

    slot_iterator & operator++() { // preincrement
        SDL_ASSERT(slot_index < parent_size());
        ++slot_index;
        return (*this);
    }
    slot_iterator operator++(int) { // postincrement
        auto temp = *this;
        ++(*this);
        //SDL_ASSERT(temp != *this);
        return temp;
    }
    slot_iterator & operator--() { // predecrement
        SDL_ASSERT(slot_index);
        --slot_index;
        return (*this);
    }
    slot_iterator operator--(int) { // postdecrement
        auto temp = *this;
        --(*this);
        //SDL_ASSERT(temp != *this);
        return temp;
    }
    bool operator==(const slot_iterator& it) const {
        SDL_ASSERT(!parent || !it.parent || (parent == it.parent));
        return
            (parent == it.parent) &&
            (slot_index == it.slot_index);
    }
    bool operator!=(const slot_iterator& it) const {
        return !(*this == it);
    }
    value_type operator*() const {
        SDL_ASSERT(slot_index < parent_size());
        return (*parent)[slot_index];
    }
private:
    // normally should return value_type * 
    // but it needs to store current value_type as class member (e.g. value_type m_cur)
    // note: value_type can be movable only 
    value_type operator->() const {
        return **this;
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
#if 0 // without iterator
    template<class fun_type>
    const_pointer find_if(fun_type fun) const {
        const size_t sz = slot.size();
        for (size_t i = 0; i < sz; ++i) {
            if (auto p = (*this)[i]) {
                if (fun(p)) {
                    return p;
                }
            }
        }
        return nullptr; // row not found
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
#else
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
#endif
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

} // db
} // sdl

#endif // __SDL_SYSTEM_DATAPAGE_H__
