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
    class index_page { // intermediate level
        spatial_tree const * tree;
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
    using datapage = datapage_t<spatial_page_row>; // leaf level
private:
    /*class data_access: noncopyable { // leaf level
        spatial_tree * const tree;
    public:
        using iterator = page_iterator<data_access, datapage>;
        using value_type = datapage const *;
        explicit data_access(spatial_tree * p) : tree(p){}
        iterator begin();
        iterator end();
    private:
        friend iterator;
        value_type dereference(datapage const & p) {
            return &p;
        }
        void load_next(datapage & p) {
            tree->load_next_page(p);
        }
        void load_prev(datapage & p) {
            tree->load_prev_page(p);
        }
        bool is_end(datapage const &) const;
    };*/
    /*class data_page_access: noncopyable {  // leaf level
        using datapage = datapage_t<spatial_page_row>;
        datapage m_data;
    public:
        using iterator = datapage::iterator;
        explicit data_page_access(page_head const * h): m_data(h) {
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
    using unique_data_page_access = std::unique_ptr<data_page_access>;*/
private:
    page_head const * load_leaf_page(bool) const;
    //datapage begin_index() const;
    //datapage end_index() const;
    //bool is_begin_index(index_page const &) const;
    //bool is_end_index(index_page const &) const;
    //void load_next_page(datapage &) const;
    //void load_prev_page(datapage &) const;
public:
    spatial_tree(database *, page_head const *, shared_primary_key const &);
    ~spatial_tree(){}

    page_head const * min_page() const; // min leaf level page
    page_head const * max_page() const; // max leaf level page

    spatial_page_row const * min_page_row() const;
    spatial_page_row const * max_page_row() const;

    spatial_cell min_cell() const;
    spatial_cell max_cell() const;

    pageFileID find_page(cell_ref) const;   // find leaf level page
    recordID begin(cell_ref) const;   // find leaf level page record, lower_bound

private:
    void for_range(spatial_cell const & c1, spatial_cell const & c2) const;
    //data_access _pages{ this }; // leaf level pages
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
