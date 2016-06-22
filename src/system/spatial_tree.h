// spatial_tree.h
//
#pragma once
#ifndef __SDL_SYSTEM_SPATIAL_TREE_H__
#define __SDL_SYSTEM_SPATIAL_TREE_H__

#include "spatial_index.h"
#include "primary_key.h"

namespace sdl { namespace db { 

class database;

class spatial_tree: noncopyable {
public:
    using key_ref = spatial_cell const &;
    using pk0_type = int64;         //FIXME: template type
private:
    //using vector_pk0 = std::vector<pk0_type>;
    using spatial_tree_error = sdl_exception_t<spatial_tree>;
private:
    using index_page_key = datapage_t<spatial_root_row>;
    using row_mem = spatial_root_row::data_type;
private:
    class index_page {
        spatial_tree const * const tree;
        page_head const * head; // current-level
        size_t slot;
    public:
        index_page(spatial_tree const *, page_head const *, size_t);        
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
        bool is_key_NULL(size_t) const; // cell_id and pk0 both NULL
    private:
        friend spatial_tree;
        pageFileID const & row_page(size_t) const;
        key_ref get_key(spatial_root_row const * x) const {
            return x->data.cell_id;
        }
        key_ref row_key(size_t) const;
        size_t find_slot(key_ref) const;
    };
private:
    class page_access: noncopyable {
        spatial_tree * const tree;
    public:
        using iterator = page_iterator<page_access, index_page>;
        using value_type = index_page const *;
        explicit page_access(spatial_tree * p) : tree(p){}
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
    class datapage_access: noncopyable {
        using datapage = datapage_t<spatial_page_row>;
        datapage m_data;
    public:
        using iterator = datapage::iterator;
        explicit datapage_access(page_head const * h): m_data(h) {
            SDL_ASSERT(h && h->is_data());
        }
        iterator begin() const {
            return m_data.begin();
        }
        iterator end() const {
            return m_data.end();
        }
        size_t size() const {
            return m_data.size();
        }
        datapage const & data() const {
            return m_data;
        }
    private:
        size_t lower_bound(key_ref) const;
    };
    using unique_datapage_access = std::unique_ptr<datapage_access>;
private:
    static bool key_less(key_ref x, key_ref y) {
        return x < y;
    }    
    page_head const * load_leaf_page(bool const begin) const;
    page_head const * page_begin() const {
        return load_leaf_page(true);
    }
    page_head const * page_end() const {
        return load_leaf_page(false);
    }
    index_page begin_index() const;
    index_page end_index() const;
    void load_next_page(index_page &) const;
    void load_prev_page(index_page &) const;
    bool is_begin_index(index_page const &) const;
    bool is_end_index(index_page const &) const;
public:
    spatial_tree(database *, page_head const *, shared_primary_key const &);
    ~spatial_tree(){}

    spatial_cell min_cell() const;
    spatial_cell max_cell() const;

    pageFileID find_page(key_ref) const;    // returns page that contains spatial_page_row(s)
    recordID lower_bound(key_ref) const;
    //recordID upper_bound(key_ref) const;
    //spatial_page_row const * get_record(recordID);    
    unique_datapage_access get_datapage(key_ref) const;

    page_access _pages{ this };
private:
    void for_range(spatial_cell const & c1, spatial_cell const & c2) const;
private:
    database * const this_db;
    page_head const * const cluster_root;
    spatial_grid const grid;
};

using unique_spatial_tree = std::unique_ptr<spatial_tree>;

} // db
} // sdl

#include "spatial_tree.inl"

#endif // __SDL_SYSTEM_SPATIAL_TREE_H__
