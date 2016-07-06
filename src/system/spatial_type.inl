// spatial_type.inl
//
#pragma once
#ifndef __SDL_SYSTEM_SPATIAL_TYPE_INL__
#define __SDL_SYSTEM_SPATIAL_TYPE_INL__

namespace sdl { namespace db {

inline int spatial_cell::compare(spatial_cell const & x, spatial_cell const & y) {
    SDL_ASSERT(x.data.depth <= size);
    SDL_ASSERT(y.data.depth <= size);
    A_STATIC_ASSERT_TYPE(uint8, id_type);
    uint8 count = a_min(x.data.depth, y.data.depth);
    const uint8 * p1 = x.data.id;
    const uint8 * p2 = y.data.id;
    int v;
    while (count--) {
        if ((v = int_diff(*(p1++), *(p2++))) != 0) {
            return v;
        }
    }
    return int_diff(x.data.depth, y.data.depth);
}
inline bool spatial_cell::equal(spatial_cell const & x, spatial_cell const & y) {
    SDL_ASSERT(x.data.depth <= size);
    SDL_ASSERT(y.data.depth <= size);
    A_STATIC_ASSERT_TYPE(uint8, id_type);
    if (x.data.depth != y.data.depth)
        return false;
    uint8 count = x.data.depth;
    const uint8 * p1 = x.data.id;
    const uint8 * p2 = y.data.id;
    while (count--) {
        if (*(p1++) != *(p2++)) {
            return false;
        }
    }
    return true;
}
inline bool operator == (spatial_point const & x, spatial_point const & y) { 
    return x.equal(y); 
}
inline bool operator != (spatial_point const & x, spatial_point const & y) { 
    return !(x == y);
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