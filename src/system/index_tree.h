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
    using page_slot = std::pair<page_head const *, size_t>;
    using page_stack = std::vector<page_slot>;
public:
    using row_mem_type = std::pair<mem_range_t, pageFileID>;
    using key_mem = mem_range_t;
private:
    class index_access { // index tree traversal
        friend index_tree;
        index_tree const * const tree;
        page_stack stack; // upper-level
        page_head const * head; // current-level
        size_t slot;
    public:
        explicit index_access(index_tree const *, size_t i = 0);
        bool operator == (index_access const & x) const {
           SDL_ASSERT(tree == x.tree);
            return (head == x.head) && (slot == x.slot);
        }
        bool operator != (index_access const & x) const {
            return !((*this) == x);
        }
    private:
        using index_page_char = datapage_t<index_page_row_char>;
        size_t size() const { return slot_array::size(head); }
        key_mem get_key(index_page_row_char const *) const;
        key_mem row_key(size_t) const;
        row_mem_type row_data(size_t) const;
        pageFileID row_page(size_t) const;
        size_t find_slot(key_mem) const;
        pageFileID find_page(key_mem) const;
        bool is_key_NULL(size_t) const;
        void push_stack(page_head const *);
    };
    void load_next(index_access&);
    bool is_begin(index_access const &) const;    
    bool is_end(index_access const &) const;
    index_access get_begin();
    index_access get_end();
    row_mem_type dereference(index_access const & p) {
        return p.row_data(p.slot);
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

    bool key_less(key_mem, key_mem) const;
    pageFileID find_page(key_mem);
    
    template<class T> 
    pageFileID find_page_t(T const & key);
public:
    page_stack const & get_stack(iterator const &) const; // diagnostic
    size_t get_slot(iterator const &) const; // diagnostic
private:
    database * const db;
    size_t const key_length;
    unique_cluster_index cluster;
};

template<class T>
pageFileID index_tree::find_page_t(T const & key) {
    if (index().size() == 1) {
        if (index()[0].type == key_to_scalartype<T>::value) {
            const char * const p = reinterpret_cast<const char *>(&key);
            return find_page({p, p + sizeof(T)});
        }
    }
    SDL_ASSERT(0);
    return{};
}

using unique_index_tree = std::unique_ptr<index_tree>;

} // db
} // sdl

#endif // __SDL_SYSTEM_INDEX_TREE_H__
