// math_util.cpp
//
#include "common/common.h"
#include "math_util.h"

namespace sdl { namespace db {

bool math_util::line_rect_intersect(point_2D const & a, point_2D const & b, rect_2D const & rc) {
    point_2D const & lb = rc.lb();
    if (line_intersect(a, b, rc.lt, lb)) return true;
    if (line_intersect(a, b, lb, rc.rb)) return true;
    point_2D const & rt = rc.rt();
    if (line_intersect(a, b, rc.rb, rt)) return true;
    if (line_intersect(a, b, rt, rc.lt)) return true;
    return false;
}

math_util::contains_t
math_util::contains(vector_point_2D const & cont, rect_2D const & rc)
{
    SDL_ASSERT(!cont.empty());
    SDL_ASSERT(rc.lt < rc.rb);
    SDL_ASSERT(frange(rc.lt.X, 0, 1));
    SDL_ASSERT(frange(rc.lt.Y, 0, 1));
    SDL_ASSERT(frange(rc.rb.X, 0, 1));
    SDL_ASSERT(frange(rc.rb.Y, 0, 1));
    auto end = cont.end();
    auto first = end - 1;
    bool crossing = false;
    for (auto second = cont.begin(); second != end; ++second) {
        point_2D const & p1 = *first;   // vert[j]
        point_2D const & p2 = *second;  // vert[i]
        if (line_rect_intersect(p1, p2, rc)) {
            return contains_t::intersect;
        }
        if (ray_crossing(rc.lt, p1, p2)) {
            crossing = !crossing; // At each crossing, the ray switches between inside and outside
        }
        first = second;
    }
    // no intersection between contour and rect
    if (crossing) {
        return contains_t::rect_inside;
    }
    if (point_inside(cont[0], rc)) { // test any point of contour
        return contains_t::poly_inside;
    }
    return contains_t::none;
}

double math_util::min_x(vector_point_2D const & p) {
    SDL_ASSERT(!p.empty());
    double x = std::numeric_limits<double>::max();
    for (auto & it : p) {
        x = a_min(x, it.X);
    }
    return x;
}

double math_util::min_y(vector_point_2D const & p) {
    SDL_ASSERT(!p.empty());
    double y = std::numeric_limits<double>::max();
    for (auto & it : p) {
        y = a_min(y, it.Y);
    }
    return y;
}

double math_util::max_x(vector_point_2D const & p) {
    SDL_ASSERT(!p.empty());
    double x = std::numeric_limits<double>::min();
    for (auto & it : p) {
        x = a_max(x, it.X);
    }
    return x;
}

double math_util::max_y(vector_point_2D const & p) {
    SDL_ASSERT(!p.empty());
    double y = std::numeric_limits<double>::min();
    for (auto & it : p) {
        y = a_max(y, it.Y);
    }
    return y;
}

bool math_util::sorted_X(vector_point_2D const & v) {
    return std::is_sorted(v.begin(), v.end(), [](point_2D const & lh, point_2D const & rh) {
        return lh.X < rh.X;
    });
}

bool math_util::sorted_Y(vector_point_2D const & v) {
    return std::is_sorted(v.begin(), v.end(), [](point_2D const & lh, point_2D const & rh) {
        return lh.Y < rh.Y;
    });
}

} // db
} // sdl
