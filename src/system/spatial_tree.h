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
    using spatial_tree_error = sdl_exception_t<spatial_tree>;
    using pk0_type = int64;         //FIXME: template type
    using vector_pk0 = std::vector<pk0_type>;
public:
    spatial_tree(database *, page_head const *, shared_primary_key const &);
    ~spatial_tree();

    spatial_cell min_cell() const;
    spatial_cell max_cell() const;

    using cell_range = std::pair<spatial_cell, spatial_cell>;
    vector_pk0 find(cell_range const &) const;
private:
    class data_t;
    std::unique_ptr<data_t> m_data;
};

using unique_spatial_tree = std::unique_ptr<spatial_tree>;

} // db
} // sdl

#endif // __SDL_SYSTEM_SPATIAL_TREE_H__
