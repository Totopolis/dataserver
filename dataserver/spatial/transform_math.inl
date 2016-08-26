// transform_math.inl
//
#pragma once
#ifndef __SDL_SYSTEM_TRANSFORM_MATH_INL__
#define __SDL_SYSTEM_TRANSFORM_MATH_INL__

namespace sdl { namespace db { namespace space { 

inline const point_2D & math::sector_point(sector_t const s) {
    return (s.h == hemisphere::north) ? north_quadrant[s.q] : south_quadrant[s.q];
}

inline math::quadrant operator++(math::quadrant t) {
    return static_cast<math::quadrant>(static_cast<int>(t)+1);
}

inline bool operator == (math::sector_t const & x, math::sector_t const & y) { 
    return (x.h == y.h) && (x.q == y.q);
}

inline bool operator != (math::sector_t const & x, math::sector_t const & y) { 
    return !(x == y);
}

inline double math::norm_longitude(double x) { // wrap around meridian +/-180
    return spatial_point::norm_longitude(x);        
}

inline double math::norm_latitude(double x) { // wrap around poles +/-90
    return spatial_point::norm_latitude(x);        
}

inline double math::add_longitude(double const lon, double const d) {
    SDL_ASSERT(SP::valid_longitude(lon));
    return norm_longitude(lon + d);
}

inline double math::add_latitude(double const lat, double const d) {
    SDL_ASSERT(SP::valid_latitude(lat));
    return norm_latitude(lat + d);
}

// The shape of the Earth is well approximated by an oblate spheroid with a polar radius of 6357 km and an equatorial radius of 6378 km. 
// PROVIDED a spherical approximation is satisfactory, any value in that range will do, such as R (in km) = 6378 - 21 * sin(lat)
inline double math::earth_radius(double const lat, ellipsoid_true) {
    SDL_ASSERT(SP::valid_latitude(lat));
    const double rad = a_abs(lat) * limits::DEG_TO_RAD;
    return limits::EARTH_MAJOR_RADIUS - (limits::EARTH_MAJOR_RADIUS - limits::EARTH_MINOR_RADIUS) * a_max(0.0, std::sin(rad));
}

inline double math::earth_radius(double const lat1 , double const lat2, ellipsoid_true) {
    SDL_ASSERT(SP::valid_latitude(lat1));
    SDL_ASSERT(SP::valid_latitude(lat2));
    return earth_radius((lat1 + lat2) / 2, ellipsoid_true{});
}

inline math::quadrant math::longitude_quadrant(Longitude const x) {
    return longitude_quadrant(x.value());
}

inline math::sector_t math::spatial_sector(spatial_point const & p) {
    return {  
        latitude_hemisphere(p.latitude),
        longitude_quadrant(p.longitude) };
}

inline point_2D math::project_globe(spatial_point const & s) {
    return math::project_globe(s, latitude_hemisphere(s));
}

inline Meters math::haversine(spatial_point const & p1, spatial_point const & p2)
{
    return haversine(p1, p2, earth_radius(p1.latitude, p2.latitude));
}

inline Meters math::haversine_error(spatial_point const & p1, spatial_point const & p2, Meters const radius)
{
    return a_abs(haversine(p1, p2).value() - radius.value());
}

inline double math::longitude_distance(double const lon1, double const lon2, bool const change_direction) {
    const double d = longitude_distance(lon1, lon2);
    return change_direction ? -d : d;
}

namespace globe_to_cell_ {

inline int min_max(const double p, const int _max) {
    return a_max<int>(a_min<int>(static_cast<int>(p), _max), 0);
};

inline point_XY<int> min_max(const point_2D & p, const int _max) {
    return{
        a_max<int>(a_min<int>(static_cast<int>(p.X), _max), 0),
        a_max<int>(a_min<int>(static_cast<int>(p.Y), _max), 0)
    };
};

inline point_XY<int> multiply_grid(point_XY<int> const & p, int const grid) {
    return { p.X * grid, p.Y * grid };
}

inline point_2D fraction(const point_2D & pos_0, const point_XY<int> & h_0, const int g_0) {
    SDL_ASSERT((g_0 > 0) && is_power_two(g_0));
    return {
        g_0 * (pos_0.X - h_0.X * 1.0 / g_0),
        g_0 * (pos_0.Y - h_0.Y * 1.0 / g_0)
    };
}
inline point_2D scale(const int scale, const point_2D & pos_0) {
    SDL_ASSERT((scale > 0) && is_power_two(scale));
    return {
        scale * pos_0.X,
        scale * pos_0.Y
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

} // globe_to_cell_

inline XY math::rasterization(point_2D const & p, const int max_id) {
    return{
        globe_to_cell_::min_max(max_id * p.X, max_id - 1),
        globe_to_cell_::min_max(max_id * p.Y, max_id - 1)
    };
}

inline void math::rasterization(rect_XY & dest, rect_2D const & src, spatial_grid const grid) {
    const int max_id = grid.s_3();
    dest.lt = rasterization(src.lt, max_id);
    dest.rb = rasterization(src.rb, max_id);
}


} // space
} // db
} // sdl

#endif // __SDL_SYSTEM_TRANSFORM_MATH_INL__