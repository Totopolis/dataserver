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

class index_tree: noncopyable
{
    using row_mem_type = std::pair<mem_range_t, pageFileID>;
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
        row_mem_type get() const;
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
    std::string type_key(row_mem_type const &) const;

    page_head const * root() const {
        return cluster->root;
    }
    cluster_index const & index() const {
        return *cluster.get();
    }
private:
    auto dereference(index_access const & p) ->decltype(p.get()) {
        return p.get();
    }
private:
    database * const db;
    size_t const key_length;
    unique_cluster_index cluster;
};

using unique_index_tree = std::unique_ptr<index_tree>;

} // db
} // sdl

#endif // __SDL_SYSTEM_INDEX_TREE_H__
