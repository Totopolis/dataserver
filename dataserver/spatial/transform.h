// transform.h
//
#pragma once
#ifndef __SDL_SPATIAL_TRANSFORM_H__
#define __SDL_SPATIAL_TRANSFORM_H__

#include "dataserver/spatial/function_cell.h"
#include "dataserver/spatial/interval_cell.h"

namespace sdl { namespace db {

struct transform : is_static {

    using function_ref = function_cell &;
    static constexpr double infinity = std::numeric_limits<double>::max();
    using grid_size = spatial_grid::grid_size;

    static spatial_cell make_cell(spatial_point const &, spatial_grid const = {});
    static point_2D project_globe(spatial_point const &);
    static spatial_point spatial(point_2D const &);
    static spatial_point spatial(spatial_cell const &, spatial_grid const = {});
    static point_XY<int> d2xy(spatial_cell::id_type, grid_size const = grid_size::HIGH); // hilbert::d2xy
    static point_2D cell2point(spatial_cell const &, spatial_grid const = {}); // returns point inside square 1x1
    static break_or_continue cell_range(function_cell &&, spatial_point const &, Meters, spatial_grid const = {});
    static break_or_continue cell_rect(function_cell &&, spatial_rect const &, spatial_grid const = {});
    template<class fun_type> static break_or_continue cell_range_t(fun_type &&, spatial_point const &, Meters, spatial_grid const = {});    
    template<class fun_type> static break_or_continue cell_rect_t(fun_type &&, spatial_rect const &, spatial_grid const = {});
#if SDL_USE_INTERVAL_CELL
    static void old_cell_range(interval_cell &, spatial_point const &, Meters, spatial_grid const = {});
    static void old_cell_rect(interval_cell &, spatial_rect const &, spatial_grid const = {});
#endif
    static Meters STDistance(spatial_point const &, spatial_point const &);
    static Meters STDistance(spatial_point const * first, spatial_point const * end, spatial_point const &, intersect_flag);
    static bool STContains(spatial_point const * first, spatial_point const * end, spatial_point const &);
    static bool STIntersects(spatial_rect const &, spatial_point const &);
    static bool STIntersects(spatial_rect const &, spatial_point const * first, spatial_point const * end, intersect_flag);
    static Meters STLength(spatial_point const * first, spatial_point const * end);
};

struct transform_t : is_static {

    template<intersect_flag f, class T>
    static Meters STDistance(T const & obj, spatial_point const & p) {
        return transform::STDistance(obj.begin(), obj.end(), p, f);
    }
    template<class T>
    static bool STContains(T const & obj, spatial_point const & p) {
        return transform::STContains(obj.begin(), obj.end(), p);
    }
    template<intersect_flag f, class T>
    static bool STIntersects(spatial_rect const & rc, T const & obj) {
        return transform::STIntersects(rc, obj.begin(), obj.end(), f);
    }
    template<class T>
    static bool STIntersects(spatial_rect const & rc, T const & obj, intersect_flag f) {
        return transform::STIntersects(rc, obj.begin(), obj.end(), f);
    }
    template<class T>
    static Meters STLength(T const & obj) {
        return transform::STLength(obj.begin(), obj.end());
    }
};


inline spatial_point transform::spatial(spatial_cell const & cell, spatial_grid const grid) {
    return transform::spatial(transform::cell2point(cell, grid));
}

template<class fun_type> inline break_or_continue
transform::cell_range_t(fun_type && fun, spatial_point const & where, Meters const radius, spatial_grid const grid) {
    return transform::cell_range(function_cell_t<fun_type>(std::move(fun)), where, radius, grid);
}

template<class fun_type> inline break_or_continue
transform::cell_rect_t(fun_type && fun, spatial_rect const & where, spatial_grid const grid) {
    return transform::cell_rect(function_cell_t<fun_type>(std::move(fun)), where, grid);
}

} // db
} // sdl

#endif // __SDL_SPATIAL_TRANSFORM_H__