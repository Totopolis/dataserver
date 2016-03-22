// index_tree_t.h
//
#ifndef __SDL_SYSTEM_INDEX_TREE_T_H__
#define __SDL_SYSTEM_INDEX_TREE_T_H__

#pragma once

#include "datapage.h"
#include "primary_key.h"

namespace sdl { namespace db {

class database;

//FIXME: template<typename key_type>
class index_tree_t: noncopyable {
    using index_tree_error = sdl_exception_t<index_tree_t>;
    using page_slot = std::pair<page_head const *, size_t>;
    using index_page_char = datapage_t<index_page_row_char>;
public:
    using row_mem = std::pair<mem_range_t, pageFileID>;
    using key_mem = mem_range_t;
private:
    class index_page {
        index_tree_t const * const tree;
        page_head const * head; // current-level
        size_t slot;
    public:
        index_page(index_tree_t const *, page_head const *, size_t);        
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
    private:
        friend index_tree_t;
        key_mem get_key(index_page_row_char const *) const;
        key_mem row_key(size_t) const;
        pageFileID const & row_page(size_t) const;
        size_t find_slot(key_mem) const;
        pageFileID find_page(key_mem) const;
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
private:
    class row_access: noncopyable {
        index_tree_t * const tree;
    public:
        using iterator = page_iterator<row_access, index_page>; 
        using value_type = row_mem;
        explicit row_access(index_tree_t * p) : tree(p){}
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
        bool is_end(index_page const &) const;
    };
private:
    class page_access: noncopyable {
        index_tree_t * const tree;
    public:
        using iterator = page_iterator<page_access, index_page>;
        using value_type = index_page const *;
        explicit page_access(index_tree_t * p) : tree(p){}
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
    index_tree_t(database *, shared_cluster_index const &);
    ~index_tree_t(){}

    //std::string type_key(key_mem) const;

    page_head const * root() const {
        return cluster->root();
    }
    cluster_index const & index() const {
        return *cluster.get();
    }
    bool key_less(key_mem, key_mem) const;
    bool key_less(vector_mem_range_t const &, key_mem) const;
    bool key_less(key_mem, vector_mem_range_t const &) const;

    pageFileID find_page(key_mem) const;
    
    template<class T> 
    pageFileID find_page_t(T const & key) const;

    row_access _rows{ this };
    page_access _pages{ this };
private:
    database * const db;
    shared_cluster_index const cluster;
    size_t const key_length;
};

//----------------------------------------------------------------------

inline bool index_tree_t::index_page::is_key_NULL() const
{
    return !(slot || head->data.prevPage);
}

inline index_tree_t::index_page
index_tree_t::begin_index() const
{
    return index_page(this, page_begin(), 0);
}

inline index_tree_t::index_page
index_tree_t::end_index() const
{
    page_head const * const h = page_end();
    return index_page(this, h, slot_array::size(h));
}

inline bool index_tree_t::is_begin_index(index_page const & p) const
{
    return !(p.slot || p.head->data.prevPage);
}

inline bool index_tree_t::is_end_index(index_page const & p) const
{
    if (p.slot == p.size()) {
        SDL_ASSERT(!p.head->data.nextPage);
        return true;
    }
    SDL_ASSERT(p.slot < p.size());
    return false;
}

template<class T> inline
pageFileID index_tree_t::find_page_t(T const & key) const {
    SDL_ASSERT(index()[0].type == key_to_scalartype<T>::value);
    const char * const p = reinterpret_cast<const char *>(&key);
    return find_page({p, p + sizeof(T)});
}

inline index_tree_t::key_mem
index_tree_t::index_page::get_key(index_page_row_char const * const row) const {
    const char * const p1 = &(row->data.key);
    const char * const p2 = p1 + tree->key_length;
    SDL_ASSERT(p1 < p2);
    return { p1, p2 };
}

inline index_tree_t::key_mem 
index_tree_t::index_page::row_key(size_t const i) const {
    return get_key(index_page_char(this->head)[i]);
}

inline pageFileID const & 
index_tree_t::index_page::row_page(size_t const i) const {
    return * reinterpret_cast<const pageFileID *>(row_key(i).second);
}

inline index_tree_t::row_mem
index_tree_t::index_page::operator[](size_t const i) const {
    auto const & m = row_key(i);
    auto const & p = * reinterpret_cast<const pageFileID *>(m.second);
    SDL_ASSERT(p);
    return { m, p };
}

inline pageFileID index_tree_t::index_page::find_page(key_mem key) const
{
    return row_page(find_slot(key)); 
}

//----------------------------------------------------------------------

inline index_tree_t::row_access::iterator
index_tree_t::row_access::begin()
{
    return iterator(this, tree->begin_index());
}

inline index_tree_t::row_access::iterator
index_tree_t::row_access::end()
{
    return iterator(this, tree->end_index());
}

inline void index_tree_t::row_access::load_next(index_page & p)
{
    tree->load_next_row(p);
}

inline void index_tree_t::row_access::load_prev(index_page & p)
{
    tree->load_prev_row(p);
}

inline bool index_tree_t::row_access::is_end(index_page const & p) const
{
    return tree->is_end_index(p);
}

inline bool index_tree_t::row_access::is_key_NULL(iterator const & it) const
{
    return it.current.is_key_NULL();
}

//----------------------------------------------------------------------

inline index_tree_t::page_access::iterator
index_tree_t::page_access::begin()
{
    return iterator(this, tree->begin_index());
}

inline index_tree_t::page_access::iterator
index_tree_t::page_access::end()
{
    return iterator(this, tree->end_index());
}

inline void index_tree_t::page_access::load_next(index_page & p)
{
    tree->load_next_page(p);
}

inline void index_tree_t::page_access::load_prev(index_page & p)
{
    tree->load_prev_page(p);
}

inline bool index_tree_t::page_access::is_end(index_page const & p) const
{
    return tree->is_end_index(p);
}

//----------------------------------------------------------------------

} // db
} // sdl

#endif // __SDL_SYSTEM_INDEX_TREE_T_H__
