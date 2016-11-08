// math_util.h
//
#pragma once
#ifndef __SDL_SYSTEM_MATH_UTIL_H__
#define __SDL_SYSTEM_MATH_UTIL_H__

#include "dataserver/spatial/spatial_type.h"

namespace sdl { namespace db {

using pair_size_t = std::pair<size_t, size_t>;

struct math_util : is_static {

    static bool get_line_intersect(point_2D &,
        point_2D const & a, point_2D const & b,  // line1 (a,b)
        point_2D const & c, point_2D const & d); // line2 (c,d)

    static bool line_intersect(
        point_2D const & a, point_2D const & b,  // line1 (a,b)
        point_2D const & c, point_2D const & d); // line2 (c,d)

    static bool line_rect_intersect(point_2D const & a, point_2D const & b, rect_2D const &);
    static bool polyline_rect_intersect(point_2D const * first, point_2D const * end, rect_2D const &);
    enum class contains_t {
        none,
        intersect,
        rect_inside,
        poly_inside
    };
    static contains_t contains(point_2D const * first, point_2D const * end, rect_2D const &);
    static contains_t contains(vector_point_2D const &, rect_2D const &);
    static bool polygon_intersect(spatial_point const * first, spatial_point const * end, spatial_rect const &);
    static bool linestring_intersect(spatial_point const * first, spatial_point const * end, spatial_rect const &);
    static bool point_inside(point_2D const & p, rect_2D const & rc);

    template<class iterator, class fun_type>
    static pair_size_t find_range(iterator first, iterator last, fun_type less);

    template<class rect_type, class point_type>
    static void update_bbox(rect_type &, point_type const &);

    template<class rect_type, class iterator>
    static void get_bbox(rect_type &, iterator first, iterator end);

    static bool point_in_polygon(
        spatial_point const * first,
        spatial_point const * last,
        spatial_point const & test);

    static bool point_in_polygon(
        spatial_point const * first,
        spatial_point const * last,
        spatial_point const & test,
        bool & point_on_vertix);

    static winding ring_winding(spatial_point const * first, spatial_point const * last);
    template<class T> static winding ring_winding(T const & ring) {
        return ring_winding(ring.begin(), ring.end());
    }
};

inline bool math_util::point_inside(point_2D const & p, rect_2D const & rc) {
    SDL_ASSERT(!(rc.rb < rc.lt));    
    return (p.X >= rc.lt.X) && (p.X <= rc.rb.X) &&
           (p.Y >= rc.lt.Y) && (p.Y <= rc.rb.Y);
}

template<class iterator, class fun_type>
pair_size_t math_util::find_range(iterator first, iterator last, fun_type less) {
    SDL_ASSERT(first != last);
    auto p1 = first;
    auto p2 = p1;
    auto it = first; ++it;
    for  (; it != last; ++it) {
        if (less(*it, *p1)) {
            p1 = it;
        }
        else if (less(*p2, *it)) {
            p2 = it;
        }
    }
    return { p1 - first, p2 - first };
}

template<class rect_type, class point_type> inline
void math_util::update_bbox(rect_type & rc, point_type const & p) {
    set_min(rc.lt.X, p.X);
    set_min(rc.lt.Y, p.Y);
    set_max(rc.rb.X, p.X);
    set_max(rc.rb.Y, p.Y);
}

template<class rect_type, class iterator>
void math_util::get_bbox(rect_type & rc, iterator first, iterator end) {
    SDL_ASSERT(first != end);
    rc.lt = rc.rb = *(first++);
    for (; first != end; ++first) {
        update_bbox(rc, *first);
    }
    SDL_ASSERT(rc.lt.X <= rc.rb.X);
    SDL_ASSERT(rc.lt.Y <= rc.rb.Y);
    SDL_ASSERT(!(rc.rb < rc.lt));
}

inline math_util::contains_t
math_util::contains(vector_point_2D const & cont, rect_2D const & rc)
{
    return math_util::contains(cont.data(), cont.data() + cont.size(), rc);
}

} // db
} // sdl

#endif // __SDL_SYSTEM_MATH_UTIL_H__