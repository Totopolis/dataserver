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
    using key_type = spatial_key_t<pk0_type>;
    using key_ref = key_type const &;
    using cell_ref = spatial_cell const &;
    using row_ref = spatial_tree_row::data_type const &;
    using spatial_index = datapage_t<spatial_tree_row>; // index_page_key
private:
    static bool is_index(page_head const *);
    static bool is_data(page_head const *);
    using spatial_datapage = datapage_t<spatial_page_row>; // leaf level
    using unique_datapage = std::unique_ptr<spatial_datapage>;
    class datapage_access: noncopyable {
        using state_type = page_head const *;
        spatial_tree * const tree;
    public:
        using iterator = page_iterator<datapage_access, state_type>;
        explicit datapage_access(spatial_tree * p): tree(p){
            SDL_ASSERT(tree);
        }
        iterator begin();
        iterator end();
    private:
        friend iterator;
        unique_datapage dereference(state_type p) {
            return make_unique<spatial_datapage>(p);
        }
        void load_next(state_type &);
        void load_prev(state_type &);
        bool is_end(state_type p) const {
            return nullptr == p;
        }
    };
private:
    page_head const * load_leaf_page(bool) const;
public:
    spatial_tree(database *, page_head const *, shared_primary_key const &);
    ~spatial_tree(){}

    datapage_access _datapage{ this }; // leaf level pages

    page_head const * min_page() const; // min leaf level page
    page_head const * max_page() const; // max leaf level page

    spatial_page_row const * min_page_row() const;
    spatial_page_row const * max_page_row() const;

    spatial_cell min_cell() const;
    spatial_cell max_cell() const;

    recordID find(cell_ref) const;

    template<class fun_type>
    void for_range(spatial_cell const &, spatial_cell const &, fun_type) const;
private:
    static size_t find_slot(spatial_index const &, cell_ref);
    static bool intersect(spatial_page_row const *, cell_ref);
    static bool is_front_intersect(page_head const *, cell_ref);
    static bool is_back_intersect(page_head const *, cell_ref);
    page_head const * page_lower_bound(cell_ref) const;
    pageFileID find_page(cell_ref) const;
    //spatial_page_row const * load_record(recordID const &) const;
private:
    using spatial_tree_error = sdl_exception_t<spatial_tree>;
    database * const this_db;
    page_head const * const cluster_root;
    mutable page_head const * _min_page = nullptr;
    mutable page_head const * _max_page = nullptr;
};

template<class fun_type>
void spatial_tree::for_range(spatial_cell const & c1, spatial_cell const & c2, fun_type fun) const
{
    SDL_ASSERT(c1 && c2);
    SDL_ASSERT((c1 == c2) || !c1.intersect(c2));
    if (!(c2 < c1)) {
        auto const r1 = find(c1);
        /*if (r1.first) {
            auto const r2 = (c1 < c2) ? find(c2) : r1;
            if (r2.first) {
            }
        }*/
    }
    else {
        SDL_ASSERT(0);
    }
}

using unique_spatial_tree = std::unique_ptr<spatial_tree>;

} // db
} // sdl

#endif // __SDL_SYSTEM_SPATIAL_TREE_H__
