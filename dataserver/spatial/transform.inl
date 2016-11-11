// transform.h
//
#pragma once
#ifndef __SDL_SYSTEM_TRANSFORM_INL__
#define __SDL_SYSTEM_TRANSFORM_INL__

#include "dataserver/spatial/math_util.h"

namespace sdl { namespace db { namespace space { 

inline constexpr double scalar_mul(point_3D const & p1, point_3D const & p2) {
    return p1.X * p2.X + p1.Y * p2.Y + p1.Z * p2.Z;
}
inline constexpr double scalar_mul(point_2D const & p1, point_2D const & p2) {
    return p1.X * p2.X + p1.Y * p2.Y;
}
inline constexpr point_3D multiply(point_3D const & p, double const d) {
    return { p.X * d, p.Y * d, p.Z * d };
}
inline constexpr point_2D multiply(point_2D const & p, double const d) {
    return { p.X * d, p.Y * d };
}
inline constexpr point_3D minus_point(point_3D const & p1, point_3D const & p2) { // = p1 - p2
    return { p1.X - p2.X, p1.Y - p2.Y, p1.Z - p2.Z };
}
inline constexpr point_2D minus_point(point_2D const & p1, point_2D const & p2) { // = p1 - p2
    return { p1.X - p2.X, p1.Y - p2.Y };
}
inline constexpr bool point_on_plane(const point_3D & p, const point_3D & V0, const point_3D & N) {
    return fequal(scalar_mul(N, minus_point(p, V0)), 0.0);
}
inline double length(const point_3D & p) {
    return std::sqrt(scalar_mul(p, p));
}
inline double length(const point_2D & p) {
    return std::sqrt(scalar_mul(p, p));
}
inline double distance(const point_2D & p1, const point_2D & p2) {
    return length(minus_point(p1, p2));
}
inline double distance(const point_3D & p1, const point_3D & p2) {
    return length(minus_point(p1, p2));
}
inline point_3D normalize(const point_3D & p) {
    const double d = length(p);
    SDL_ASSERT(!fzero(d));
    return multiply(p, 1.0 / d);
}
inline point_2D normalize(const point_2D & p) {
    const double d = length(p);
    SDL_ASSERT(!fzero(d));
    return multiply(p, 1.0 / d);
}
#if 0
inline double fstable(double const d) {
    double const r = (d >= 0) ? 
        (int)(d + limits::fepsilon) :
        (int)(d - limits::fepsilon);
    return fequal(d, r) ? r : d;
}
#endif
inline double vector_angle_rad(point_2D const & v1, point_2D const & v2) {
    // v1 * v2 = |v1| * |v2| * cos(a)
    // a = acos(v1 * v2 / |v1| / |v2|)
    double const d1 = length(v1);
    double const d2 = length(v2);
    if (fzero(d1) || fzero(d2)) {
        SDL_ASSERT(0);
        return 0;
    }
    return std::acos(a_min(scalar_mul(v1, v2) / d1 / d2, 1.0));
}

namespace globe_to_cell_ {

inline int min_max(const double p, const int _max) {
    return a_max<int, 0>(a_min<int>(static_cast<int>(p), _max));
};

template<int _max>
inline int min_max(const double p) {
    return a_max<int, 0>(a_min<int, _max>(static_cast<int>(p)));
};

inline point_XY<int> min_max(const point_2D & p, const int _max) {
    return{
        a_max<int, 0>(a_min<int>(static_cast<int>(p.X), _max)),
        a_max<int, 0>(a_min<int>(static_cast<int>(p.Y), _max))
    };
};

template<const int _max>
inline point_XY<int> min_max(const point_2D & p) {
    return{
        a_max<int, 0>(a_min<int, _max>(static_cast<int>(p.X))),
        a_max<int, 0>(a_min<int, _max>(static_cast<int>(p.Y)))
    };
};

inline point_2D fraction(const point_2D & pos_0, const point_XY<int> & h_0, const int g_0) {
    SDL_ASSERT((g_0 > 0) && is_power_two(g_0));
    return {
        g_0 * (pos_0.X - h_0.X * 1.0 / g_0),
        g_0 * (pos_0.Y - h_0.Y * 1.0 / g_0)
    };
}

template<int g_0>
inline point_2D fraction(const point_2D & pos_0, const point_XY<int> & h_0) {
    static_assert(g_0 == 16, "spatial_grid::HIGH");
    static_assert((g_0 > 0) && is_power_two(g_0), "");
    return {
        g_0 * (pos_0.X - h_0.X * 1.0 / g_0),
        g_0 * (pos_0.Y - h_0.Y * 1.0 / g_0)
    };
}


inline point_2D scale(const int scalefactor, const point_2D & pos_0) {
    SDL_ASSERT((scalefactor > 0) && is_power_two(scalefactor));
    return {
        scalefactor * pos_0.X,
        scalefactor * pos_0.Y
    };
}

template<const int scalefactor>
inline point_2D scale(const point_2D & pos_0) {
    static_assert((scalefactor > 0) && is_power_two(scalefactor), "");
    return {
        scalefactor * pos_0.X,
        scalefactor * pos_0.Y
    };
}

inline XY mod_XY(const XY & pos_0, const point_XY<int> & h_0, const int g_0) {
    SDL_ASSERT((g_0 > 0) && is_power_two(g_0));
    SDL_ASSERT(h_0.X * g_0 <= pos_0.X);
    SDL_ASSERT(h_0.Y * g_0 <= pos_0.Y);
    return {
         pos_0.X - h_0.X * g_0, 
         pos_0.Y - h_0.Y * g_0
    };
}
inline XY div_XY(const XY & pos_0, const int g_0) {
    SDL_ASSERT((g_0 > 0) && is_power_two(g_0));
    return {
         pos_0.X / g_0, 
         pos_0.Y / g_0
    };
}

template<const int g_0>
inline XY mod_XY(const XY & pos_0, const point_XY<int> & h_0) {
    static_assert((g_0 > 0) && is_power_two(g_0), "mod_XY");
    SDL_ASSERT(h_0.X * g_0 <= pos_0.X);
    SDL_ASSERT(h_0.Y * g_0 <= pos_0.Y);
    return {
         pos_0.X - h_0.X * g_0, 
         pos_0.Y - h_0.Y * g_0
    };
}

template<const int g_0>
inline XY div_XY(const XY & pos_0) {
    static_assert((g_0 > 0) && is_power_two(g_0), "div_XY");
    return {
         pos_0.X / g_0, 
         pos_0.Y / g_0
    };
}

#if SDL_DEBUG > 1 // reserved
inline size_t remain(size_t const x, size_t const y) {
    size_t const d = x % y;
    return d ? (y - d) : 0;
}
inline size_t roundup(double const x, size_t const y) {
    SDL_ASSERT(x >= 0);
    size_t const d = a_max(static_cast<size_t>(x + 0.5), y); 
    return d + remain(d, y); 
}
#endif

template<size_t const y>
inline size_t remain(size_t const x) {
    static_assert(y && is_power_2<y>::value, "");
    size_t const d = x & (y - 1);
    return d ? (y - d) : 0;
}
template<size_t const y>
inline size_t roundup(double const x) {
    SDL_ASSERT(x >= 0);
    size_t const d = a_max(static_cast<size_t>(x + 0.5), y); 
    return d + remain<y>(d); 
}

} // globe_to_cell_

namespace rasterization_ {

#if high_grid_optimization
template<int max_id>
inline XY rasterization(point_2D const & p) {
    return{
        globe_to_cell_::min_max<max_id - 1>(max_id * p.X),
        globe_to_cell_::min_max<max_id - 1>(max_id * p.Y)
    };
}

inline void rasterization(rect_XY & dest, rect_2D const & src, spatial_grid const grid) {
    dest.lt = rasterization<grid.s_3()>(src.lt);
    dest.rb = rasterization<grid.s_3()>(src.rb);
}

template<class buf_XY, class buf_2D>
void rasterization(buf_XY & dest, buf_2D const & src, spatial_grid const grid) {
    SDL_ASSERT(dest.empty());
    SDL_ASSERT(!src.empty());
    XY val;
    for (auto const & p : src) {
        val = rasterization<grid.s_3()>(p);
        if (dest.empty() || (val != dest.back())) {
            dest.push_back(val);
        }
    }
}
#else
inline XY rasterization(point_2D const & p, const int max_id) {
    return{
        globe_to_cell_::min_max(max_id * p.X, max_id - 1),
        globe_to_cell_::min_max(max_id * p.Y, max_id - 1)
    };
}

inline void rasterization(rect_XY & dest, rect_2D const & src, spatial_grid const grid) {
    const int max_id = grid.s_3();
    dest.lt = rasterization(src.lt, max_id);
    dest.rb = rasterization(src.rb, max_id);
}

template<class buf_XY, class buf_2D>
void rasterization(buf_XY & dest, buf_2D const & src, spatial_grid const grid) {
    SDL_ASSERT(dest.empty());
    SDL_ASSERT(!src.empty());
    const int max_id = grid.s_3();
    XY val;
    for (auto const & p : src) {
        val = rasterization(p, max_id);
        if (dest.empty() || (val != dest.back())) {
            dest.push_back(val);
        }
    }
}
#endif

inline void get_bbox(rect_XY & bbox, 
                     point_2D const * const first,
                     point_2D const * const end,
                     spatial_grid const grid) {
    SDL_ASSERT(first < end);
    rect_2D bbox_2D;
    math_util::get_bbox(bbox_2D, first, end);
    rasterization(bbox, bbox_2D, grid);
}

} // rasterization_
} // space
} // db
} // sdl

#endif // __SDL_SYSTEM_TRANSFORM_INL__