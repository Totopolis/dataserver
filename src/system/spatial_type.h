// spatial_type.h
//
#pragma once
#ifndef __SDL_SYSTEM_SPATIAL_TYPE_H__
#define __SDL_SYSTEM_SPATIAL_TYPE_H__

#include "page_type.h"

namespace sdl { namespace db {

namespace unit {
    struct Latitude{};
    struct Longitude{};
}
typedef quantity<unit::Latitude, double> Latitude;      // in degrees
typedef quantity<unit::Longitude, double> Longitude;    // in degrees

enum class spatial_type {
    null = 0,
    point = 0x0C01,
    multipolygon = 0x0401,
    linestring = 0x1401
};

//template<spatial_type T>
//using spatial_t = Val2Type<spatial_type, T>;

#pragma pack(push, 1) 

struct spatial_cell { // 5 bytes
    
    static const size_t size = 4;
    using id_type = uint8;

    struct data_type { // 5 bytes
        id_type id[size];
        id_type depth;   // [1..4]
    };
    union {
        data_type data;
        char raw[sizeof(data_type)];
    };
    id_type operator[](size_t i) const {
        SDL_ASSERT(i < size);
        return data.id[i];
    }
    id_type & operator[](size_t i) {
        SDL_ASSERT(i < size);
        return data.id[i];
    }
    bool is_null() const {
        SDL_ASSERT(data.depth <= size);
        return 0 == data.depth;
    }
    explicit operator bool() const {
        return !is_null();
    }
    size_t depth() const {
        SDL_ASSERT(data.depth <= size);
        return (data.depth <= size) ? size_t(data.depth) : size;
    }
    void depth(size_t const d) {
        SDL_ASSERT(d && (d <= 4));
        data.depth = (id_type)a_min<size_t>(d, 4);
    }
    static spatial_cell min();
    static spatial_cell max();
    static spatial_cell parse_hex(const char *);
    static bool intersect(spatial_cell const &, spatial_cell const &);
    bool intersect(spatial_cell const & y) const {
        return intersect(*this, y);
    }
};

inline double min_latitude()    { return -90; }
inline double max_latitude()    { return 90; }
inline double min_longitude()   { return -180; }
inline double max_longitude()   { return 180; }

struct spatial_point { // 16 bytes

    double latitude;
    double longitude;

    static bool is_valid(Latitude const d) {
        return fless_equal(d.value(), max_latitude()) && fless_equal(min_latitude(), d.value());
    }
    static bool is_valid(Longitude const d) {
        return fless_equal(d.value(), max_longitude()) && fless_equal(min_longitude(), d.value());
    }
    bool is_valid() const {
        return is_valid(Latitude(this->latitude)) && is_valid(Longitude(this->longitude));
    }
    static spatial_point init(Latitude const lat, Longitude const lon) {
        SDL_ASSERT(is_valid(lat) && is_valid(lon));
        return { lat.value(), lon.value() };
    }
    static spatial_point STPointFromText(const std::string &); // POINT (longitude latitude)
};

template<typename T>
struct point_XY {
    using type = T;
    type X, Y;
};

template<typename T>
struct point_XYZ {
    using type = T;
    type X, Y, Z;
};

#pragma pack(pop)

inline bool operator < (spatial_point const & x, spatial_point const & y) {
    if (x.latitude < y.latitude) return true;
    if (y.latitude < x.latitude) return false;
    return x.longitude < y.longitude;
}
inline bool operator == (spatial_point const & x, spatial_point const & y) { 
    return (x.latitude == y.latitude) && (x.longitude == y.longitude); 
}
inline bool operator != (spatial_point const & x, spatial_point const & y) { 
    return !(x == y);
}
bool operator < (spatial_cell const & x, spatial_cell const & y);
bool operator == (spatial_cell const & x, spatial_cell const & y);
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

struct spatial_grid {
    enum grid_size : uint8 {
        LOW     = 4,    // 4X4,     16 cells
        MEDIUM  = 8,    // 8x8,     64 cells
        HIGH    = 16    // 16x16,   256 cells
    };
    grid_size level[4];
    explicit spatial_grid(
        grid_size const s0,
        grid_size const s1  = grid_size::HIGH,
        grid_size const s2  = grid_size::HIGH,
        grid_size const s3  = grid_size::HIGH) {
        level[0] = s0; level[1] = s1;
        level[2] = s2; level[3] = s3;
    }
    spatial_grid() {
        for (auto & l : level) {
            l = grid_size::HIGH;
        }
    }
    int operator[](size_t i) const {
        SDL_ASSERT(i < A_ARRAY_SIZE(level));
        return level[i];
    }
};

struct spatial_transform : is_static {
    static spatial_cell make_cell(spatial_point const &, spatial_grid const & g = {});
    static spatial_cell make_cell(Latitude lat, Longitude lon, spatial_grid const & g  = {}) {
        return make_cell(spatial_point::init(lat, lon), g);
    }
    static point_XY<int> make_XY(spatial_cell const &, spatial_grid::grid_size); // for diagnostics (hilbert::d2xy)
    static point_XY<double> point(spatial_cell const &, spatial_grid const & g = {}); // for diagnostics (point inside square 1x1)
};

} // db
} // sdl

#endif // __SDL_SYSTEM_SPATIAL_TYPE_H__