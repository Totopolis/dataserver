// datapage.h
//
#pragma once
#ifndef __SDL_SYSTEM_DATAPAGE_H__
#define __SDL_SYSTEM_DATAPAGE_H__

#include "dataserver/system/page_head.h"
#include "dataserver/sysobj/boot_page.h"
#include "dataserver/sysobj/file_header.h"
#include "dataserver/sysobj/sysallocunits.h"
#include "dataserver/sysobj/sysschobjs.h"
#include "dataserver/sysobj/syscolpars.h"
#include "dataserver/sysobj/sysidxstats.h"
#include "dataserver/sysobj/sysscalartypes.h"
#include "dataserver/sysobj/sysobjvalues.h"
#include "dataserver/sysobj/sysiscols.h"
#include "dataserver/sysobj/sysrowsets.h"
#include "dataserver/sysobj/pfs_page.h"
#include "dataserver/system/index_page.h"
#include "dataserver/spatial/spatial_index.h"
#include "dataserver/system/slot_iterator.h"
#include "dataserver/system/page_iterator.h"
#include "dataserver/system/database_fwd.h"
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

inline pfs_byte pfs_page::operator[](pageFileID const & id) const {
    SDL_ASSERT(!id.is_null());
    return (*row)[id.pageId % pfs_size];
}

template<class T>
struct datapage_name {
    static const char * name() {
        return "";
    }
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
    static const char * name() { // optional
        return datapage_name<datapage_t>::name();
    }
public:
    static constexpr size_t none_slot = size_t(-1);
    page_head const * const head;
    slot_array const slot;

    explicit datapage_t(page_head const * h): head(h), slot(h) {
        SDL_ASSERT(head);
        static_assert(sizeof(row_type) < page_head::body_size, "");
        //SDL_ASSERT(sizeof(row_type) <= head->data.pminlen); possible for system tables ?
        SDL_ASSERT(!empty());
    }
    virtual ~datapage_t(){}

    bool empty() const {
        return slot.empty();
    }
    explicit operator bool() const {
        return !empty();
    }
    size_t size() const {
        return slot.size();
    }
    template<size_t i> const_pointer get_row(Size2Type<i>) const {
        return cast::page_row<row_type>(this->head, this->slot.get(Size2Type<i>{}));
    }
    const_pointer operator[](size_t const i) const {
        return cast::page_row<row_type>(this->head, this->slot[i]);
    }
    pageFileID const & prevPage() const {
        return head->data.prevPage;
    }
    pageFileID const & nextPage() const {
        return head->data.nextPage;
    }
    const_pointer front() const {
        return get_row(Size2Type<0>{});
    }
    const_pointer back() const {
        return (*this)[size()-1];
    }
    row_type const & at(size_t const i) const { // throw_error if empty
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
    const_pointer find_if(fun_type && fun) const {
        auto const last = this->end();
        auto const it = std::find_if(this->begin(), last, fun);
        if (it != last) {
            return *it;
        }
        return nullptr; // row not found
    }
    template<class fun_type>
    void for_row(fun_type && fun) const {
        for (auto p : *this) {
            A_STATIC_CHECK_TYPE(const_pointer, p);
            fun(p);
        }
    }
    template<class fun_type>
    void scan_if(fun_type && fun) const {
        for (auto p : *this) {
            A_STATIC_CHECK_TYPE(const_pointer, p);
            if (!fun(p)) {
                break;
            }
        }
    }
    iterator begin_slot(const size_t pos) const {
        SDL_ASSERT(pos <= size());
        return iterator(this, pos);
    }
    template<class fun_type>
    size_t lower_bound(fun_type &&) const;
    
    //template<class value_type, class compare_fun> 
    //size_t btree_index(const value_type& value, compare_fun) const;
};

template<class T> template<class fun_type>
size_t datapage_t<T>::lower_bound(fun_type && less) const {
    size_t count = this->size();
    size_t first = 0;
    SDL_ASSERT(count);
    while (count) {
        const size_t count2 = count / 2;
        const size_t mid = first + count2;
        if (less((*this)[mid])) {
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

/*template<class T>
template<class value_type, class compare_fun>
size_t datapage_t<T>::btree_index(const value_type& value, compare_fun comp) const {
    size_t count = this->size();
    size_t first = 0;
    SDL_ASSERT(count);
    while (count) {
        const size_t count2 = count / 2;
        const size_t mid = first + count2;
        if (comp((*this)[mid], value)) { // row[mid] < value
            first = mid + 1;
            count -= count2 + 1;
        }
        else {
            count = count2;
        }
    }
    SDL_ASSERT(first <= this->size());
    if (first && (first < this->size())) {
        if (comp(value, (*this)[first])) { // value < row[first]
            return first - 1;
        }
        return first;
    }
    return first - 1;
}*/

class sysallocunits : public datapage_t<sysallocunits_row> {
    typedef datapage_t<sysallocunits_row> base_type;
public:
    explicit sysallocunits(page_head const * h) : base_type(h) {}
    const_pointer find_auid(uint32) const; // find (first) row with auid
    template<class fun_type> 
    void scan_auid(uint32, fun_type &&) const;
};

inline sysallocunits::const_pointer
sysallocunits::find_auid(uint32 const id) const {
    A_STATIC_CHECK_TYPE(decltype(auid_t().d.id) const, id);
    A_STATIC_ASSERT_TYPE(const_pointer, sysallocunits_row const *);
    return find_if([id](sysallocunits_row const * const p) {
        return (p->data.auid.d.id == id); //FIXME: can meet multiple auid.id with different auid.hi
    });
}

template<class fun_type> inline
void sysallocunits::scan_auid(uint32 const id, fun_type && fun) const {
    this->scan_if([id, &fun](sysallocunits_row const * const p){
        if (p->data.auid.d.id == id) { 
            return is_break(fun(p));
        }
        return true;
    });
}
//----------------------------------------------------------------------------

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

#define define_datapage_name(classname) \
template<> struct datapage_name< datapage_t<classname##_row> > { \
    static const char * name() { return #classname; } };

define_datapage_name(fileheader)
define_datapage_name(sysallocunits)
define_datapage_name(sysschobjs)
define_datapage_name(syscolpars)
define_datapage_name(sysidxstats)
define_datapage_name(sysscalartypes)
define_datapage_name(sysobjvalues)
define_datapage_name(sysiscols)
define_datapage_name(sysrowsets)
    
} // db
} // sdl

#endif // __SDL_SYSTEM_DATAPAGE_H__
