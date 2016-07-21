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
inline bool operator < (spatial_cell const & x, spatial_cell const & y) {
    return spatial_cell::compare(x, y) < 0;
}
inline bool operator == (spatial_cell const & x, spatial_cell const & y) {
    return spatial_cell::equal(x, y);
}
inline bool operator != (spatial_cell const & x, spatial_cell const & y) {
    return !(x == y);
}
//------------------------------------------------------------------------------------
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
} // db
} // sdl

#endif // __SDL_SYSTEM_SPATIAL_TYPE_INL__