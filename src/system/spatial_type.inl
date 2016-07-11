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
inline bool operator < (spatial_cell const & x, spatial_cell const & y) {
    return spatial_cell::compare(x, y) < 0;
}
inline bool operator == (spatial_cell const & x, spatial_cell const & y) {
    return spatial_cell::equal(x, y);
}
inline bool operator != (spatial_cell const & x, spatial_cell const & y) {
    return !(x == y);
}
template<typename T>
inline bool operator == (point_XY<T> const & p1, point_XY<T> const & p2) {
    return fequal(p1.X, p2.X) && fequal(p1.Y, p2.Y);
}
template<typename T>
inline bool operator != (point_XY<T> const & p1, point_XY<T> const & p2) {
    return !(p1 == p2);
}
template<typename T>
inline bool operator == (point_XYZ<T> const & p1, point_XYZ<T> const & p2) {
    return fequal(p1.X, p2.X) && fequal(p1.Y, p2.Y) && fequal(p1.Z, p2.Z);
}
template<typename T>
inline bool operator != (point_XYZ<T> const & p1, point_XYZ<T> const & p2) {
    return !(p1 == p2);
}

} // db
} // sdl

#endif // __SDL_SYSTEM_SPATIAL_TYPE_INL__