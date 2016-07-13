// transform.h
//
#pragma once
#ifndef __SDL_SYSTEM_TRANSFORM_H__
#define __SDL_SYSTEM_TRANSFORM_H__

#include "spatial_type.h"

namespace sdl { namespace db {

struct transform : is_static {
    using grid_size = spatial_grid::grid_size;
    static spatial_cell make_cell(spatial_point const &, spatial_grid const = {});
    static point_XY<int> d2xy(spatial_cell::id_type, grid_size const = grid_size::HIGH); // hilbert::d2xy
    static point_2D point(spatial_cell const &, spatial_grid const = {}); // returns point inside square 1x1 // point
    static vector_cell cell_range(spatial_point const &, Meters, spatial_grid const = {});
    //FIXME: cell_rect
};

} // db
} // sdl

#endif // __SDL_SYSTEM_TRANSFORM_H__