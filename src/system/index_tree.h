// index_tree.h
//
#pragma once
#ifndef __SDL_SYSTEM_INDEX_TREE_H__
#define __SDL_SYSTEM_INDEX_TREE_H__

#include "datapage.h"
#include "primary_key.h"

namespace sdl { namespace db {

class database;

class index_tree: noncopyable {
    using index_tree_error = sdl_exception_t<index_tree>;
    using page_slot = std::pair<page_head const *, size_t>;
    using index_page_row_char = index_page_row_t<char>;
    using index_page_char = datapage_t<index_page_row_char>;
public:
    using row_mem = std::pair<mem_range_t, pageFileID>;
    using key_mem = mem_range_t;
private:
    class index_page {
        index_tree const * const tree;
        page_head const * head; // current-level
        size_t slot;
    public:
        index_page(index_tree const *, page_head const *, size_t);        
        bool operator == (index_page const & x) const {
           SDL_ASSERT(tree == x.tree);
           return (head == x.head) && (slot == x.slot);
        }
        page_head const * get_head() const {
            return this->head;
        }
        bool operator != (index_page const & x) const {
            return !((*this) == x);
        }
        size_t size() const { 
            return slot_array::size(head);
        }
        row_mem operator[](size_t i) const;
        pageFileID const & min_page() const;
        pageFileID const & max_page() const;
    private:
        friend index_tree;
        key_mem get_key(index_page_row_char const *) const;
        key_mem row_key(size_t) const;
        pageFileID const & row_page(size_t) const;
        size_t find_slot(key_mem) const;
        pageFileID const & find_page(key_mem) const;
        bool is_key_NULL() const;
    };
private:
    index_page begin_index() const;
    index_page end_index() const;
    void load_prev_row(index_page &) const;
    void load_next_row(index_page &) const;
    void load_next_page(index_page &) const;
    void load_prev_page(index_page &) const;
    bool is_begin_index(index_page const &) const;
    bool is_end_index(index_page const &) const;
    page_head const * load_leaf_page(bool begin) const;
    page_head const * page_begin() const {
        return load_leaf_page(true);
    }
    page_head const * page_end() const {
        return load_leaf_page(false);
    }
    template<class fun_type>
    pageFileID find_page_if(fun_type) const;
private:
    class row_access: noncopyable {
        index_tree * const tree;
    public:
        using iterator = page_iterator<row_access, index_page>; 
        using value_type = row_mem;
        explicit row_access(index_tree * p) : tree(p){}
        iterator begin();
        iterator end();
        bool is_key_NULL(iterator const &) const;
        size_t slot(iterator const & it) const {
            return it.current.slot;
        }
    private:
        friend iterator;
        value_type dereference(index_page const & p) {
            return p[p.slot];
        }
        void load_next(index_page &);
        void load_prev(index_page &);
        bool is_end(index_page const & p) const {
           return tree->is_end_index(p);
        }
    };
private:
    class page_access: noncopyable {
        index_tree * const tree;
    public:
        using iterator = page_iterator<page_access, index_page>;
        using value_type = index_page const *;
        explicit page_access(index_tree * p) : tree(p){}
        iterator begin();
        iterator end();
    private:
        friend iterator;
        value_type dereference(index_page const & p) {
            return &p;
        }
        void load_next(index_page &);
        void load_prev(index_page &);
        bool is_end(index_page const &) const;
    };
    int sub_key_compare(size_t, key_mem const &, key_mem const &) const;
public:
    using row_iterator_value = row_access::value_type;
    using page_iterator_value = page_access::value_type;
public:
    index_tree(database *, shared_cluster_index const &);
    ~index_tree(){}

    page_head const * root() const {
        return cluster->root();
    }
    cluster_index const & index() const {
        return *cluster.get();
    }
    bool key_less(key_mem, key_mem) const;
    bool key_less(vector_mem_range_t const &, key_mem) const;
    bool key_less(key_mem, vector_mem_range_t const &) const;

    std::string type_key(key_mem) const; //diagnostic

    pageFileID find_page(key_mem) const;    
    
    template<class T>
    pageFileID find_page_t(T const & key) const;

    pageFileID min_page() const;
    pageFileID max_page() const;

//private:
    row_access _rows{ this };
    page_access _pages{ this };

private:
    database * const db;
    shared_cluster_index const cluster;
    size_t const key_length;
};

using unique_index_tree = std::unique_ptr<index_tree>;

} // db
} // sdl

#include "index_tree.inl"

#endif // __SDL_SYSTEM_INDEX_TREE_H__
