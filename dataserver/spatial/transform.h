// transform.h
//
#pragma once
#ifndef __SDL_SPATIAL_TRANSFORM_H__
#define __SDL_SPATIAL_TRANSFORM_H__

#include "dataserver/spatial/spatial_type.h"
#include "dataserver/spatial/interval_cell.h"

namespace sdl { namespace db {

struct transform : is_static {
#if SDL_DEBUG
    class function_cell {
        mutable size_t call_count[4] = {};
        virtual break_or_continue process(spatial_cell const cell) const = 0;
    protected:
        ~function_cell() {
            for (size_t i = 0; i < count_of(call_count); ++i) {
                SDL_TRACE("function_cell[", i, "] = ", call_count[i]);
            }
        }
    public:
        break_or_continue operator()(spatial_cell const cell) const {
            ++call_count[cell.data.depth-1];
            return process(cell);
        }
    };
#else
    class function_cell {
        virtual break_or_continue process(spatial_cell const cell) const = 0;
    protected:
        ~function_cell() = default;
    public:
        break_or_continue operator()(spatial_cell const cell) const {
            return process(cell);
        }
    };
#endif
    template<class fun_type>
    class function_cell_t : public function_cell {
        fun_type m_fun;
        break_or_continue process(spatial_cell const cell) const override {
            return m_fun(cell);
        }
    public:
        explicit function_cell_t(fun_type && f): m_fun(std::move(f)) {}
    };
    template<class T>
    static function_cell_t<T> make_fun(T && f) {
        return function_cell_t<T>(std::forward<T>(f));
    }
    using function_ref = function_cell const &;
    static constexpr double infinity = std::numeric_limits<double>::max();
    using grid_size = spatial_grid::grid_size;

    static spatial_cell make_cell(spatial_point const &, spatial_grid const = {});
    static point_2D project_globe(spatial_point const &);
    static spatial_point spatial(point_2D const &);
    static spatial_point spatial(spatial_cell const &, spatial_grid const = {});
    static point_XY<int> d2xy(spatial_cell::id_type, grid_size const = grid_size::HIGH); // hilbert::d2xy
    static point_2D cell2point(spatial_cell const &, spatial_grid const = {}); // returns point inside square 1x1
    static break_or_continue cell_range(function_ref, spatial_point const &, Meters, spatial_grid const = {});
    static break_or_continue cell_rect(function_ref, spatial_rect const &, spatial_grid const = {});
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

inline spatial_point transform::spatial(spatial_cell const & cell, spatial_grid const grid) {
    return transform::spatial(transform::cell2point(cell, grid));
}

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

} // db
} // sdl

#endif // __SDL_SPATIAL_TRANSFORM_H__