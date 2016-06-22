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
    using pk0_type = int64;                         //FIXME: template type ?
    using vector_pk0 = std::vector<pk0_type>;
    using key_type = spatial_tree_key_t<pk0_type>;
    using key_ref = key_type const &;
    using cell_ref = spatial_cell const &;
    using row_ref = spatial_tree_row::data_type const &;
private:
    using index_page_key = datapage_t<spatial_tree_row>;
private:
    class index_page { // copyable
        spatial_tree const * tree;
        page_head const * head; // current-level
        size_t slot;
    public:
        index_page(spatial_tree const *, page_head const *, size_t /*slot*/);        
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
        row_ref operator[](size_t const i) const {
            return index_page_key(this->head)[i]->data;
        }
        bool is_key_NULL(size_t) const; // cell_id and pk0 both NULL
    private:
        friend spatial_tree;
        pageFileID const & row_page(size_t const i) const  {
            return index_page_key(this->head)[i]->data.page;
        }
        cell_ref get_cell(spatial_tree_row const * x) const {
            return x->data.key.cell_id;
        }
        cell_ref row_cell(size_t const i) const {
           return get_cell(index_page_key(this->head)[i]);
        }
        size_t find_slot(cell_ref) const; //FIXME: lower_bound using first part of composite key (spatial_cell) 
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
        size_t lower_bound(cell_ref) const;
    };
    using unique_datapage_access = std::unique_ptr<datapage_access>;
private:
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

    pageFileID find_page(cell_ref) const;    // returns page that contains spatial_page_row(s)
    recordID lower_bound(cell_ref) const;

    unique_datapage_access get_datapage(cell_ref) const;

    page_access _pages{ this };
private:
    void for_range(spatial_cell const & c1, spatial_cell const & c2) const;
private:
    using spatial_tree_error = sdl_exception_t<spatial_tree>;
    database * const this_db;
    page_head const * const cluster_root;
    spatial_grid const grid;
};

using unique_spatial_tree = std::unique_ptr<spatial_tree>;

} // db
} // sdl

#include "spatial_tree.inl"

#endif // __SDL_SYSTEM_SPATIAL_TREE_H__
