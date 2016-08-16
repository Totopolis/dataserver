// math_util.cpp
//
#include "common/common.h"
#include "math_util.h"

namespace sdl { namespace db { namespace {

#if 0
// returns Z coordinate of vector multiplication
inline double rotate(double X1, double Y1, double X2, double Y2) {
    return X1 * Y2 - X2 * Y1;
}
#endif

// returns Z coordinate of vector multiplication
inline double rotate(point_2D const & p1, point_2D const & p2) {
    return p1.X * p2.Y - p2.X * p1.Y;
}

// https://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
inline bool ray_crossing(point_2D const & test, point_2D const & p1, point_2D const & p2) {
    return ((p1.Y > test.Y) != (p2.Y > test.Y)) &&
        ((test.X + limits::fepsilon) < ((test.Y - p2.Y) * (p1.X - p2.X) / (p1.Y - p2.Y) + p2.X));
}

} // namespace

//https://en.wikipedia.org/wiki/Line%E2%80%93line_intersection

bool math_util::get_line_intersect(point_2D & result,
    point_2D const & _1, point_2D const & _2, // line1
    point_2D const & _3, point_2D const & _4) // line2
{
    double const denominator  = (_1.X - _2.X) * (_3.Y - _4.Y) - (_1.Y - _2.Y) * (_3.X - _4.X);
    if (fzero(denominator )) {
        SDL_ASSERT(0);
        return false;
    }
    double const A = _1.X * _2.Y - _1.Y * _2.X;
    double const B = _3.X * _4.Y - _3.Y * _4.X;
    result.X = (A * (_3.X - _4.X) - (_1.X - _2.X) * B) / denominator;
    result.Y = (A * (_3.Y - _4.Y) - (_1.Y - _2.Y) * B) / denominator;
    return true;
}

bool math_util::line_intersect(
                            point_2D const & a, point_2D const & b,   // line1 (a,b)
                            point_2D const & c, point_2D const & d) { // line2 (c,d)
    const point_2D a_b = b - a;
    const point_2D c_d = d - c;
    return (fsign(rotate(a_b, c - b)) * fsign(rotate(a_b, d - b)) <= 0) &&
           (fsign(rotate(c_d, a - d)) * fsign(rotate(c_d, b - d)) <= 0);
}

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

} // db
} // sdl

#if SDL_DEBUG
namespace sdl {
    namespace db {
        namespace {
            class unit_test {
            public:
                unit_test()
                {
                    point_2D const a = { -1, 0 };
                    point_2D const b = { 1, 0 };
                    point_2D const c = { 0, -1 };
                    point_2D const d = { 0, 1 };
                    point_2D p = {-1, -1};
                    SDL_ASSERT(math_util::get_line_intersect(p, a, b, c, d));
                    SDL_ASSERT(fequal(p.X, 0));
                    SDL_ASSERT(fequal(p.Y, 0));
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG
