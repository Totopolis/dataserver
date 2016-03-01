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
    class index_page {
        index_tree const * const tree;
        page_stack stack; // upper-level
        page_head const * head; // current-level
        size_t slot;
    public:
        explicit index_page(index_tree const *, size_t i = 0);
        
        bool operator == (index_page const & x) const {
           SDL_ASSERT(tree == x.tree);
            return (head == x.head) && (slot == x.slot);
        }
        bool operator != (index_page const & x) const {
            return !((*this) == x);
        }
        size_t size() const { 
            return slot_array::size(head);
        }
        row_mem_type operator[](size_t i) const {
            return row_data(i);
        }
        page_head const * get_head() const {
            return this->head;
        }
    private:
        friend index_tree;
        using index_page_char = datapage_t<index_page_row_char>;
        key_mem get_key(index_page_row_char const *) const;
        key_mem row_key(size_t) const;
        row_mem_type row_data(size_t) const;
        row_mem_type row_data() const { 
            return row_data(this->slot);
        }
        pageFileID row_page(size_t) const;
        size_t find_slot(key_mem) const;
        pageFileID find_page(key_mem) const;
        bool key_NULL(size_t) const;
        void push_stack(page_head const *);
    };
private:
    index_page begin_row();
    index_page end_row();
    bool is_begin_row(index_page const &) const;    
    bool is_end_row(index_page const &) const;
    void load_next_row(index_page &) const;
    void load_next_page(index_page &) const;
private:
    class row_access: noncopyable {
        index_tree * const tree;
    public:
        using iterator = forward_iterator<row_access, index_page>;
        explicit row_access(index_tree * p) : tree(p){}
        iterator begin() {
            return iterator(this, tree->begin_row());
        }
        iterator end() {
            return iterator(this, tree->end_row());
        }
        bool key_NULL(iterator const & it) const {
           return it.current.key_NULL(it.current.slot);
        }
        page_stack const & get_stack(iterator const & it) const { // diagnostic
            return it.current.stack;
        }
        size_t get_slot(iterator const & it) const { // diagnostic
            return it.current.slot;
        }
    private:
        friend iterator;
        row_mem_type dereference(index_page const & p) {
            return p.row_data();
        }
        void load_next(index_page& p) {
            tree->load_next_row(p);
        }
        bool is_end(index_page const & p) const {
            return tree->is_end_row(p);
        }
    };
private:
    class page_access: noncopyable {
        index_tree * const tree;
    public:
        using iterator = forward_iterator<page_access, index_page>;
        explicit page_access(index_tree * p) : tree(p){}
        iterator begin() {
            return iterator(this, tree->begin_row());
        }
        iterator end() {
            return iterator(this, tree->end_row());
        }
    private:
        friend iterator;
        index_page const * dereference(index_page const & p) {
            return &p;
        }
        void load_next(index_page& p) {
            tree->load_next_page(p);
        }
        bool is_end(index_page const & p) const {
            return tree->is_end_row(p);
        }
    };
public:
    index_tree(database *, unique_cluster_index &&);
    ~index_tree(){}

    std::string type_key(key_mem) const;

    page_head const * root() const {
        return cluster->root;
    }
    cluster_index const & index() const {
        return *cluster.get();
    }
    row_access _rows { this };
    page_access _pages{ this };

    bool key_less(key_mem, key_mem) const;
    pageFileID find_page(key_mem);
    
    template<class T> 
    pageFileID find_page_t(T const & key);
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
