// transform.h
//
#pragma once
#ifndef __SDL_SYSTEM_TRANSFORM_INL__
#define __SDL_SYSTEM_TRANSFORM_INL__

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
inline bool point_on_plane(const point_3D & p, const point_3D & V0, const point_3D & N) {
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
} // space
} // db
} // sdl

#endif // __SDL_SYSTEM_TRANSFORM_INL__