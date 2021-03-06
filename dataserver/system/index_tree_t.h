// index_tree_t.h
//
#pragma once
#ifndef __SDL_SYSTEM_INDEX_TREE_T_H__
#define __SDL_SYSTEM_INDEX_TREE_T_H__

#include "dataserver/system/datapage.h"
#include "dataserver/system/database_fwd.h"

namespace sdl { namespace db { namespace make {

template<typename KEY_TYPE>
class index_tree: noncopyable {
public:
    using key_type = KEY_TYPE;
    using key_ref = key_type const &;
private:
    using index_tree_error = sdl_exception_t<index_tree>;
    using page_slot = std::pair<page_head const *, size_t>;
    using index_page_row_key = index_page_row_t<key_type>;
    using index_page_key = datapage_t<index_page_row_key>;
    using first_key = decltype(key_type()._0);
public:
    using row_mem = typename index_page_row_key::data_type const &;
    static size_t const key_length = sizeof(key_type);
private:
    class index_page {
        using key_ref = typename index_tree::key_ref;
        using row_mem = typename index_tree::row_mem;
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
        recordID get_RID() const {
            return recordID::init(head->data.pageId, slot);
        }
    private:
        friend index_tree;
        key_ref get_key(index_page_row_key const *) const;
        key_ref row_key(size_t) const;
        pageFileID const & row_page(size_t) const;
        size_t find_slot(key_ref) const;
        size_t first_slot(first_key const &) const;
        pageFileID const & find_page(key_ref) const;
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
        index_tree const * const tree;
    public:
        using iterator = page_iterator<row_access const, index_page>; 
        using value_type = row_mem;
        explicit row_access(index_tree * p) : tree(p){}
        iterator begin() const;
        iterator end() const;
        bool is_key_NULL(iterator const &) const;
        size_t slot(iterator const & it) const {
            return it.current.slot;
        }
    private:
        friend iterator;
        static value_type dereference(index_page const & p) {
            return p[p.slot];
        }
        void load_next(index_page &) const;
        void load_prev(index_page &) const;
        bool is_end(index_page const & p) const {
           return tree->is_end_index(p);
        }
        recordID get_RID(iterator const & it) const {
            return it.current.get_RID();
        }
    };
private:
    class page_access: noncopyable {
        index_tree const * const tree;
    public:
        using iterator = page_iterator<page_access const, index_page>;
        using value_type = index_page const *;
        explicit page_access(index_tree * p) : tree(p){}
        iterator begin() const;
        iterator end() const;
    private:
        friend iterator;
        static value_type dereference(index_page const & p) {
            return &p;
        }
        void load_next(index_page &) const;
        void load_prev(index_page &) const;
        bool is_end(index_page const &) const;
    };
public:
    using row_iterator_value = typename row_access::value_type;
    using page_iterator_value = typename page_access::value_type;
public:
    index_tree(database const *, page_head const *);
    ~index_tree(){}

    page_head const * root() const { return cluster_root; }

private:
    static bool key_less(key_ref x, key_ref y) {
        return key_type::this_clustered::is_less(x, y);
    }
    static bool less_first(first_key const & x, first_key const & y) {
        return key_type::this_clustered::less_first(x, y);
    }
    template<typename make_query_type>
    pageFileID first_page_clustered(first_key const &, make_query_type const &, bool_constant<false>) const;
    template<typename make_query_type>
    pageFileID first_page_clustered(first_key const &, make_query_type const &, bool_constant<true>) const;
public:
    recordID get_RID(typename row_access::iterator const & it) const {
        return _rows.get_RID(it);
    }
    pageFileID find_page(key_ref) const;

    template<typename make_query_type>
    pageFileID first_page(first_key const &, make_query_type const &) const;

    pageFileID min_page() const;
    pageFileID max_page() const;

    row_access const _rows{ this };
    page_access const _pages{ this };

private:
    database const * const this_db;
    page_head const * const cluster_root;
};

} // make
} // db
} // sdl

#include "dataserver/system/index_tree_t.inl"
#include "dataserver/system/index_tree_t.hpp"

#endif // __SDL_SYSTEM_INDEX_TREE_T_H__
