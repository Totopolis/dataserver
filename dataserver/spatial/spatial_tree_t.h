// spatial_tree_t.h
//
#pragma once
#ifndef __SDL_SYSTEM_SPATIAL_TREE_T_H__
#define __SDL_SYSTEM_SPATIAL_TREE_T_H__

#include "spatial_index.h"
#include "transform.h"
#include "system/primary_key.h"
#include "system/database_fwd.h"

namespace sdl { namespace db {

template<typename KEY_TYPE>
class spatial_tree_t: noncopyable {
public:
    using pk0_type = KEY_TYPE;
    using key_type = spatial_key_t<pk0_type>;
    using spatial_tree_row = index_page_row_t<key_type>;
    using spatial_page_row = spatial_page_row_t<key_type>;
private:
    using key_ref = key_type const &;
    using cell_ref = spatial_cell const &;
    using row_ref = typename spatial_tree_row::data_type const &;
    using spatial_index = datapage_t<spatial_tree_row>;
private:
    static bool is_index(page_head const *);
    static bool is_data(page_head const *);
    using spatial_datapage = datapage_t<spatial_page_row>; // leaf level
    using unique_datapage = std::unique_ptr<spatial_datapage>;
    class datapage_access: noncopyable {
        using state_type = page_head const *;
        spatial_tree_t * const tree;
    public:
        using iterator = page_iterator<datapage_access, state_type>;
        explicit datapage_access(spatial_tree_t * p): tree(p){
            SDL_ASSERT(tree);
        }
        iterator begin() {
            page_head const * p = tree->min_page();
            return iterator(this, std::move(p));
        }
        iterator end() {
            return iterator(this, nullptr);
        }
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
    spatial_tree_t(database *, page_head const *, shared_primary_key const &, sysidxstats_row const *);
    ~spatial_tree_t(){}

    std::string name() const {
        return idxstat->name();
    }
    datapage_access _datapage{ this }; // leaf level pages

    page_head const * min_page() const; // min leaf level page
    page_head const * max_page() const; // max leaf level page

    spatial_page_row const * min_page_row() const;
    spatial_page_row const * max_page_row() const;

    spatial_cell min_cell() const;
    spatial_cell max_cell() const;

    recordID find_cell(cell_ref) const; // lower bound
    spatial_page_row const * load_page_row(recordID const &) const;

    template<class fun_type>
    break_or_continue for_cell(cell_ref, fun_type) const;
   
    template<class fun_type>
    break_or_continue for_point(spatial_point const & p, fun_type f) const {
        return for_cell(transform::make_cell(p), f);
    }
    template<class fun_type>
    break_or_continue for_range(spatial_point const &, Meters, fun_type) const; //for_distance

    template<class fun_type>
    break_or_continue for_rect(spatial_rect const &, fun_type) const;

    template<class fun_type>
    break_or_continue full_globe(fun_type) const;
private:
    static size_t find_slot(spatial_index const &, cell_ref);
    static bool intersect(spatial_page_row const *, cell_ref);
    static bool is_front_intersect(page_head const *, cell_ref);
    static bool is_back_intersect(page_head const *, cell_ref);
    page_head const * page_lower_bound(cell_ref) const;
    pageFileID find_page(cell_ref) const;
private:
    using spatial_tree_error = sdl_exception_t<spatial_tree_t>;
    database * const this_db;
    page_head const * const cluster_root;
    sysidxstats_row const * const idxstat;
    mutable page_head const * _min_page = nullptr;
    mutable page_head const * _max_page = nullptr;
};

template<class T>
struct tree_spatial_page_row {
    using type = typename remove_reference_t<T>::spatial_page_row; 
};

template<class T>
using tree_spatial_page_row_t = typename tree_spatial_page_row<T>::type;

} // db
} // sdl

#include "spatial_tree_t.hpp"

#endif // __SDL_SYSTEM_SPATIAL_TREE_T_H__
