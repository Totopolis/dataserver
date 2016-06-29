// spatial_tree.h
//
#pragma once
#ifndef __SDL_SYSTEM_SPATIAL_TREE_H__
#define __SDL_SYSTEM_SPATIAL_TREE_H__

#include "spatial_index.h"
#include "primary_key.h"

#define spatial_tree_use_function   0

namespace sdl { namespace db { 

class database;

class spatial_tree: noncopyable { //FIXME: spatial_tree_t<pk0_type>
public:
    using pk0_type = int64; //FIXME: template type ?
private:
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
#if spatial_tree_use_function
    using function_row = std::function<bool(spatial_page_row const *)>;
    void _for_range(spatial_cell const &, spatial_cell const &, function_row) const;
#endif
public:
    spatial_tree(database *, page_head const *, shared_primary_key const &, sysidxstats_row const *);
    ~spatial_tree(){}

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

    recordID find(cell_ref) const; // lower bound
    spatial_page_row const * load_page_row(recordID const &) const;

#if spatial_tree_use_function
    template<class fun_type>
    void for_range(spatial_cell const & c1, spatial_cell const & c2, fun_type f) const {
        _for_range(c1, c2, f);
    }
    template<class fun_type>
    void for_point(spatial_point const & p, fun_type f) const {
        spatial_cell const c = spatial_transform::make_cell(p);
        _for_range(c, c, f);
    }
#else
    template<class fun_type>
    void for_range(spatial_cell const & c1, spatial_cell const & c2, fun_type f) const;   
   
    template<class fun_type>
    void for_point(spatial_point const & p, fun_type f) const {
        spatial_cell const c = spatial_transform::make_cell(p);
        for_range(c, c, f);
    }
#endif
private:
    static size_t find_slot(spatial_index const &, cell_ref);
    static bool intersect(spatial_page_row const *, cell_ref);
    static bool is_front_intersect(page_head const *, cell_ref);
    static bool is_back_intersect(page_head const *, cell_ref);
    page_head const * page_lower_bound(cell_ref) const;
    pageFileID find_page(cell_ref) const;
    recordID load_next_record(recordID const &) const;
private:
    using spatial_tree_error = sdl_exception_t<spatial_tree>;
    database * const this_db;
    page_head const * const cluster_root;
    sysidxstats_row const * const idxstat;
    mutable page_head const * _min_page = nullptr;
    mutable page_head const * _max_page = nullptr;
};

using shared_spatial_tree = std::shared_ptr<spatial_tree>;

#if !spatial_tree_use_function
template<class fun_type>
void spatial_tree::for_range(spatial_cell const & c1, spatial_cell const & c2, fun_type fun) const
{
    SDL_ASSERT(c1 && c2);
    SDL_ASSERT((c1 == c2) || !c1.intersect(c2));
    if (!(c2 < c1)) {
        recordID it = find(c1);
        if (it) {
            while (spatial_page_row const * const p = load_page_row(it)) {
                auto const & row_cell = p->data.cell_id;
                if ((row_cell < c2) || row_cell.intersect(c2)) {
                    if (fun(p)) {
                        it = this->load_next_record(it);
                        continue;
                    }
                }
                break;
            }
        }
    }
    else {
        SDL_ASSERT(0);
    }
}
#endif
} // db
} // sdl

#endif // __SDL_SYSTEM_SPATIAL_TREE_H__
