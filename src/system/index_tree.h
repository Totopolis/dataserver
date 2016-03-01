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
    using index_tree_error = sdl_exception_t<index_tree>;
    using row_mem_type = std::pair<mem_range_t, pageFileID>;
    using page_slot = std::pair<page_head const *, size_t>;
    using page_stack = std::vector<page_slot>;
    using key_mem = mem_range_t;
private:
    class index_access { // index tree traversal
        friend index_tree;
        index_tree const * const tree;
        page_stack stack; // upper-level
        page_head const * head; // current-level
        size_t slot;
        using index_page_char = datapage_t<index_page_row_char>;
    public:
        explicit index_access(index_tree const *, size_t i = 0);
        bool operator == (index_access const & x) const {
           SDL_ASSERT(tree == x.tree);
            return (head == x.head) && (slot == x.slot);
        }
        bool operator != (index_access const & x) const {
            return !((*this) == x);
        }
        key_mem row_key(index_page_row_char const *) const;
        row_mem_type row_data() const;
        pageFileID row_page(size_t) const;
        pageFileID find_page(key_mem) const;
        bool is_key_NULL(size_t) const;
        size_t size() const {
            return slot_array::size(head);
        }
    public:
        page_stack const & get_stack() const { return this->stack; }
        size_t get_slot() const { return this->slot; }
    };
    void load_next(index_access&);
    bool is_begin(index_access const &) const;    
    bool is_end(index_access const &) const;
    index_access get_begin();
    index_access get_end();
    row_mem_type dereference(index_access const & p) {
        return p.row_data();
    }
public:
    using iterator = forward_iterator<index_tree, index_access>;
    friend iterator;

    index_tree(database *, unique_cluster_index &&);
    ~index_tree(){}

    std::string type_key(key_mem) const;

    page_head const * root() const {
        return cluster->root;
    }
    cluster_index const & index() const {
        return *cluster.get();
    }
    iterator begin() {
        return iterator(this, get_begin());
    }
    iterator end() {
        return iterator(this, get_end());
    }
    bool is_key_NULL(iterator const &) const;
public:
    pageFileID find_page(key_mem);
    bool key_less(key_mem, key_mem) const;
public:
    page_stack const & get_stack(iterator const &) const; // diagnostic
    size_t get_slot(iterator const &) const; // diagnostic
private:
    database * const db;
    size_t const key_length;
    unique_cluster_index cluster;
};

using unique_index_tree = std::unique_ptr<index_tree>;

} // db
} // sdl

#endif // __SDL_SYSTEM_INDEX_TREE_H__
