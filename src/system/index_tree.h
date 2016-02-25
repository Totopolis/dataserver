// index_tree.h
//
#ifndef __SDL_SYSTEM_INDEX_TREE_H__
#define __SDL_SYSTEM_INDEX_TREE_H__

#pragma once

#include "datapage.h"
#include "page_iterator.h"

namespace sdl { namespace db {

class database;

class index_tree_base: noncopyable {
    database * const db;
public:
    page_head const * const root;
protected:
    class index_access { // level index scan
        friend index_tree_base;
        index_tree_base const * parent;
        page_head const * head;
        size_t slot_index;
    public:
        index_access(index_tree_base const * p, page_head const * h, size_t i = 0)
            : parent(p), head(h), slot_index(i)
        {
            SDL_ASSERT(parent && head);
            SDL_ASSERT(slot_index <= slot_array::size(head));
        }
        bool operator == (index_access const & x) const {
            return (head == x.head) && (slot_index == x.slot_index);
        }
        template<class index_page_row>
        index_page_row const * dereference() const {
            SDL_ASSERT(head->data.pminlen == sizeof(index_page_row));
            return datapage_t<index_page_row>(head)[slot_index];
        }
    };
    void load_next(index_access&);
    void load_prev(index_access&);
    bool is_end(index_access const &);
    bool is_begin(index_access const &);    
    index_access get_begin();
    index_access get_end();
protected:
    index_tree_base(database *, page_head const *);
    ~index_tree_base(){}
};

//--------------------------------------------------------------------------

template<scalartype::type ST>
class index_tree_t: public index_tree_base {
public:
    using key_type = index_key<ST>;
    using row_type = index_page_row_t<key_type>;
    using row_pointer = row_type const *;
public:
    using iterator = page_iterator<index_tree_t, index_access>;
    friend iterator;

    index_tree_t(database * p, page_head const * h)
        : index_tree_base(p, h)
    {
    }
    iterator begin() {
        return iterator(this, get_begin());
    }
    iterator end() {
        return iterator(this, get_end());
    }
private:
    row_type const * dereference(index_access const & p) {
        return p.dereference<row_type>();
    }
};

//--------------------------------------------------------------------------
#if 0
class index_tree: public index_tree_base {
public:
    using iterator = page_iterator<index_tree, index_access>;
    friend iterator;

    scalartype::type const key_type;
    index_tree(database * p, page_head const * h, scalartype::type t)
        : index_tree_base(p, h)
        , key_type(t)
    {}
    iterator begin() {
        return iterator(this, get_begin());
    }
    iterator end() {
        return iterator(this, get_end());
    }
private:
    pageFileID const & dereference(index_access const & p) const;
};
#endif
//------------------------------------------------------------------

} // db
} // sdl

#endif // __SDL_SYSTEM_INDEX_TREE_H__
