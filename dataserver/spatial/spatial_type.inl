// spatial_type.inl
//
#pragma once
#ifndef __SDL_SYSTEM_SPATIAL_TYPE_INL__
#define __SDL_SYSTEM_SPATIAL_TYPE_INL__

namespace sdl { namespace db {

inline bool operator == (spatial_point const & x, spatial_point const & y) { 
    return x.equal(y); 
}
inline bool operator != (spatial_point const & x, spatial_point const & y) { 
    return !(x == y);
}
inline bool operator < (spatial_point const & x, spatial_point const & y) { 
    if (x.longitude < y.longitude) return true;
    if (y.longitude < x.longitude) return false;
    return x.latitude < y.latitude;
}
inline spatial_point operator - (spatial_point const & p1, spatial_point const & p2) {
    return spatial_point::init(
        Latitude(p1.latitude - p2.latitude),
        Longitude(p1.longitude - p2.longitude));
}
//------------------------------------------------------------------------------------
inline bool operator == (spatial_cell::id_array const & x, spatial_cell::id_array const & y) {
    return x._32 == y._32;
}
inline bool operator != (spatial_cell::id_array const & x, spatial_cell::id_array const & y) {
    return !(x == y);
}
inline bool operator < (spatial_cell const & x, spatial_cell const & y) {
    return spatial_cell::less(x, y);
}
inline bool operator == (spatial_cell const & x, spatial_cell const & y) {
    return spatial_cell::equal(x, y);
}
inline bool operator != (spatial_cell const & x, spatial_cell const & y) {
    return !(x == y);
}
inline spatial_cell spatial_cell::init(uint32 const _32, id_type const depth) {
    SDL_ASSERT(depth <= size);
    spatial_cell cell;
    cell.data.id._32 = _32;
    cell.data.depth = depth;
    SDL_ASSERT(cell.zero_tail());
    return cell;
}
//------------------------------------------------------------------------------------
inline bool operator == (point_XY<int> const & p1, point_XY<int> const & p2) {
    return (p1.X == p2.X) && (p1.Y == p2.Y);
}
inline bool operator == (point_XYZ<int> const & p1, point_XYZ<int> const & p2) {
    return (p1.X == p2.X) && (p1.Y == p2.Y) && (p1.Z == p2.Z);
}
template<typename T>
inline bool operator == (point_XY<T> const & p1, point_XY<T> const & p2) {
    static_assert(std::is_floating_point<T>::value, "");
    return fequal(p1.X, p2.X) && fequal(p1.Y, p2.Y);
}
template<typename T>
inline bool operator != (point_XY<T> const & p1, point_XY<T> const & p2) {
    return !(p1 == p2);
}
template<typename T>
inline bool operator == (point_XYZ<T> const & p1, point_XYZ<T> const & p2) {
    static_assert(std::is_floating_point<T>::value, "");
    return fequal(p1.X, p2.X) && fequal(p1.Y, p2.Y) && fequal(p1.Z, p2.Z);
}
template<typename T>
inline bool operator != (point_XYZ<T> const & p1, point_XYZ<T> const & p2) {
    return !(p1 == p2);
}
template<typename T>
inline bool operator < (point_XY<T> const & p1, point_XY<T> const & p2) { 
    if (p1.X < p2.X) return true;
    if (p2.X < p1.X) return false;
    return p1.Y < p2.Y;
}
template<typename T>
inline point_XY<T> operator + (point_XY<T> const & p1, point_XY<T> const & p2) {
    return { p1.X + p2.X, p1.Y + p2.Y };
}
template<typename T>
inline point_XYZ<T> operator + (point_XYZ<T> const & p1, point_XYZ<T> const & p2) {
    return { p1.X + p2.X, p1.Y + p2.Y, p1.Z + p2.Z };
}
template<typename T>
inline point_XY<T> operator - (point_XY<T> const & p1, point_XY<T> const & p2) {
    return { p1.X - p2.X, p1.Y - p2.Y };
}
template<typename T>
inline point_XYZ<T> operator - (point_XYZ<T> const & p1, point_XYZ<T> const & p2) {
    return { p1.X - p2.X, p1.Y - p2.Y, p1.Z - p2.Z };
}
inline bool point_frange(point_2D const & test,
                         double const x1, double const x2, 
                         double const y1, double const y2) {
    SDL_ASSERT(x1 <= x2);
    SDL_ASSERT(y1 <= y2);
    return frange(test.X, x1, x2) && frange(test.Y, y1, y2);
}
//------------------------------------------------------------------------------------
inline Degree degree(Radian const & x) {
    return limits::RAD_TO_DEG * x.value();
}
inline Radian radian(Degree const & x) {
    return limits::DEG_TO_RAD * x.value();
}
inline polar_2D polar(point_2D const & p) {
    return polar_2D::polar(p);
}
//------------------------------------------------------------------------------------
inline bool spatial_rect::is_null() const {
    SDL_ASSERT(is_valid());
    return fequal(min_lon, max_lon) || fless_eq(max_lat, min_lat);
}
inline bool spatial_rect::cross_equator() const {
    SDL_ASSERT(is_valid());
    return (min_lat < 0) && (0 < max_lat);
}
inline spatial_point spatial_rect::min() const {
    return spatial_point::init(Latitude(min_lat), Longitude(min_lon));
}
inline spatial_point spatial_rect::max() const {
    return spatial_point::init(Latitude(max_lat), Longitude(max_lon));
}
inline spatial_point spatial_rect::center() const {
    return spatial_point::init(
        Latitude((max_lat + min_lat) / 2), 
        Longitude((max_lon + min_lon) / 2));
}
inline spatial_rect spatial_rect::init(spatial_point const & p1, spatial_point const & p2) {
    spatial_rect rc;
    rc.min_lat = p1.latitude;
    rc.min_lon = p1.longitude;
    rc.max_lat = p2.latitude;
    rc.max_lon = p2.longitude;
    SDL_ASSERT(rc.is_valid());
    return rc;
}
inline spatial_rect spatial_rect::init(
                            Latitude min_lat, 
                            Longitude min_lon, 
                            Latitude max_lat, 
                            Longitude max_lon) {
    spatial_rect rc;
    rc.min_lat = min_lat.value();
    rc.min_lon = min_lon.value();
    rc.max_lat = max_lat.value();
    rc.max_lon = max_lon.value();
    SDL_ASSERT(rc.is_valid());
    return rc;
}
//------------------------------------------------------------------------------------
} // db
} // sdl

#endif // __SDL_SYSTEM_SPATIAL_TYPE_INL__