// index_tree.h
//
#ifndef __SDL_SYSTEM_INDEX_TREE_H__
#define __SDL_SYSTEM_INDEX_TREE_H__

#pragma once

#include "datapage.h"
#include "page_iterator.h"
#include "usertable.h"

namespace sdl { namespace db {

class database;

class cluster_index: noncopyable {
public:
    using shared_usertable = std::shared_ptr<usertable>;
    using column = usertable::column;
    using column_index = std::vector<size_t>; 
public:
    page_head const * const root;
    column_index const col_index;

    cluster_index(page_head const * p, column_index && c, shared_usertable const & sch)
        : root(p), col_index(std::move(c)), schema(sch)
    {
        SDL_ASSERT(root);
        SDL_ASSERT(!col_index.empty());
        SDL_ASSERT(schema.get());
    }
    size_t size() const {
        return col_index.size();
    }
    column const & operator[](size_t i) const {
        SDL_ASSERT(i < col_index.size());
        return (*schema)[col_index[i]];
    }
    size_t key_size() const; // index key(s) memory size
private:
    shared_usertable const schema;
};

using unique_cluster_index = std::unique_ptr<cluster_index>;

class index_tree: noncopyable
{
private:
    class index_access { // level index scan
        friend index_tree;
        index_tree const * parent;
        page_head const * head;
        size_t slot_index;
    public:
        index_access(index_tree const * p, page_head const * h, size_t i = 0)
            : parent(p), head(h), slot_index(i)
        {
            SDL_ASSERT(parent && head);
            SDL_ASSERT(slot_index <= slot_array::size(head));
        }
        bool operator == (index_access const & x) const {
            return (head == x.head) && (slot_index == x.slot_index);
        }
        mem_range_t get() const;
    };
    void load_next(index_access&);
    void load_prev(index_access&);
    bool is_end(index_access const &);
    bool is_begin(index_access const &);    
    index_access get_begin();
    index_access get_end();
public:
    using iterator = page_iterator<index_tree, index_access>;
    friend iterator;

    index_tree(database *, unique_cluster_index &&);
    ~index_tree(){}

    iterator begin() {
        return iterator(this, get_begin());
    }
    iterator end() {
        return iterator(this, get_end());
    }
private:
    mem_range_t dereference(index_access const & p) {
        return p.get();
    }
private:
    database * const db;
    size_t const pminlen;
    unique_cluster_index cluster;
};

using unique_index_tree = std::unique_ptr<index_tree>;

//--------------------------------------------------------------------------
#if 0
template<scalartype::type ST>
class index_tree_t: public index_tree_base {
public:
    static const scalartype::type key_value = ST;
    using key_type = index_key<ST>;
    using row_type = index_page_row_t<key_type>;
    using row_pointer = row_type const *;
public:
    using iterator = page_iterator<index_tree_t, index_access>;
    friend iterator;

    index_tree_t(database * p, cluster_index const * h)
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
        return p.get<row_type>();
    }
};
#endif

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
