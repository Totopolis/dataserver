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
#if 0
inline constexpr point_3D add_point(point_3D const & p1, point_3D const & p2) {
    return { p1.X + p2.X, p1.Y + p2.Y, p1.Z + p2.Z };
}
inline constexpr point_3D add_point(point_2D const & p1, point_2D const & p2) {
    return { p1.X + p2.X, p1.Y + p2.Y };
}
#endif
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
inline point_XY<int> min_max(const point_2D & p, const int _max) {
    return{
        a_max<int>(a_min<int>(static_cast<int>(p.X), _max), 0),
        a_max<int>(a_min<int>(static_cast<int>(p.Y), _max), 0)
    };
};
inline point_2D fraction(const point_2D & pos_0, const point_XY<int> & h_0, const int g_0) {
    return {
        g_0 * (pos_0.X - h_0.X * 1.0 / g_0),
        g_0 * (pos_0.Y - h_0.Y * 1.0 / g_0)
    };
}
inline point_2D scale(const int scale, const point_2D & pos_0) {
    return {
        scale * pos_0.X,
        scale * pos_0.Y
    };
}

} // space
} // db
} // sdl

#endif // __SDL_SYSTEM_TRANSFORM_INL__