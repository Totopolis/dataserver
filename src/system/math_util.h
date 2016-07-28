// math_util.h
//
#pragma once
#ifndef __SDL_SYSTEM_MATH_UTIL_H__
#define __SDL_SYSTEM_MATH_UTIL_H__

#include "spatial_type.h"

namespace sdl { namespace db {

struct math_util : is_static {
    // returns Z coordinate of vector multiplication
    static double rotate(double X1, double Y1, double X2, double Y2) {
        return X1 * Y2 - X2 * Y1;
    }
    // returns Z coordinate of vector multiplication
    static double rotate(point_2D const & p1, point_2D const & p2) {
        return p1.X * p2.Y - p2.X * p1.Y;
    }
    static bool point_inside(point_2D const & p, rect_2D const & rc);
    static bool line_intersect(
        point_2D const & a, point_2D const & b,  // line1 (a,b)
        point_2D const & c, point_2D const & d); // line2 (c,d)

    static bool line_rect_intersect(point_2D const & a, point_2D const & b, rect_2D const & rc);
    static bool ray_crossing(point_2D const & test, point_2D const & p1, point_2D const & p2);    
    enum class contains_t {
        none,
        intersect,
        rect_inside,
        poly_inside
    };
    static contains_t contains(vector_point_2D const &, rect_2D const &);
    static double min_x(vector_point_2D const &);
    static double min_y(vector_point_2D const &);
    static double max_x(vector_point_2D const &);
    static double max_y(vector_point_2D const &);
    static bool sorted_X(vector_point_2D const &);
    static bool sorted_Y(vector_point_2D const &);
};

inline bool math_util::line_intersect(
                            point_2D const & a, point_2D const & b,   // line1 (a,b)
                            point_2D const & c, point_2D const & d) { // line2 (c,d)
    const point_2D a_b = b - a;
    const point_2D c_d = d - c;
    return (fsign(rotate(a_b, c - b)) * fsign(rotate(a_b, d - b)) <= 0) &&
           (fsign(rotate(c_d, a - d)) * fsign(rotate(c_d, b - d)) <= 0);
}

inline bool math_util::point_inside(point_2D const & p, rect_2D const & rc) {
    SDL_ASSERT(!(rc.rb < rc.lt));    
    return (p.X >= rc.lt.X) && (p.X <= rc.rb.X) &&
           (p.Y >= rc.lt.Y) && (p.Y <= rc.rb.Y);
}

// https://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
inline bool math_util::ray_crossing(point_2D const & test, point_2D const & p1, point_2D const & p2) {
    return ((p1.Y > test.Y) != (p2.Y > test.Y)) &&
        ((test.X + limits::fepsilon) < ((test.Y - p2.Y) * (p1.X - p2.X) / (p1.Y - p2.Y) + p2.X));
}

} // db
} // sdl

#endif // __SDL_SYSTEM_MATH_UTIL_H__