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
    struct Meters{};
    struct Degree{};
    struct Radian{};
}
typedef quantity<unit::Latitude, double> Latitude;      // in degrees -90..90
typedef quantity<unit::Longitude, double> Longitude;    // in degrees -180..180
typedef quantity<unit::Meters, double> Meters;
typedef quantity<unit::Degree, double> Degree;
typedef quantity<unit::Radian, double> Radian;

enum class spatial_type {
    null = 0,
    point = 0x0C01,
    multipolygon = 0x0401,
    linestring = 0x1401
};

//template<spatial_type T> using spatial_t = Val2Type<spatial_type, T>;

#pragma pack(push, 1) 

struct spatial_cell { // 5 bytes
    
    using id_type = uint8;
    static const size_t size = 4; // max depth

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
        return data.depth;
    }
    void set_depth(size_t const d) {
        SDL_ASSERT(d && (d <= 4));
        data.depth = (id_type)a_min<size_t>(d, 4);
    }
    static spatial_cell min();
    static spatial_cell max();
    static spatial_cell parse_hex(const char *);
    bool intersect(spatial_cell const &) const;
    static int compare(spatial_cell const &, spatial_cell const &);
    static bool equal(spatial_cell const &, spatial_cell const &);
#if SDL_DEBUG
    static bool test_depth(spatial_cell const &);
#endif
};

struct spatial_point { // 16 bytes

    double latitude;
    double longitude;

    static constexpr double min_latitude    = -90;
    static constexpr double max_latitude    = 90;
    static constexpr double min_longitude   = -180;
    static constexpr double max_longitude   = 180;

    static bool valid_latitude(double const d) {
        return frange(d, min_latitude, max_latitude);
    }
    static bool valid_longitude(double const d) {
        return frange(d, min_longitude, max_longitude);
    }
    static bool is_valid(Latitude const d) {
        return valid_latitude(d.value());
    }
    static bool is_valid(Longitude const d) {
        return valid_longitude(d.value());
    }
    bool is_valid() const {
        return is_valid(Latitude(this->latitude)) && is_valid(Longitude(this->longitude));
    }
    static spatial_point init(Latitude const lat, Longitude const lon) {
        SDL_ASSERT(is_valid(lat) && is_valid(lon));
        return { lat.value(), lon.value() };
    }
    bool equal(spatial_point const & y) const {
        return fequal(latitude, y.latitude) && fequal(longitude, y.longitude); 
    }
    bool equal(Latitude const lat, Longitude const lon) const {
        return fequal(latitude, lat.value()) && fequal(longitude, lon.value()); 
    }
    bool match(spatial_point const &) const;
    static spatial_point STPointFromText(const std::string &); // POINT (longitude latitude)
};

struct spatial_grid { // 4 bytes
    enum grid_size : uint8 {
        LOW     = 4,    // 4X4,     16 cells
        MEDIUM  = 8,    // 8x8,     64 cells
        HIGH    = 16    // 16x16,   256 cells
    };
    enum { HIGH_HIGH = HIGH * HIGH };
    static const size_t size = spatial_cell::size;
    grid_size level[size];
    
    spatial_grid(): level{HIGH, HIGH, HIGH, HIGH} {}
    explicit spatial_grid(
        grid_size const s0,
        grid_size const s1 = HIGH,
        grid_size const s2 = HIGH,
        grid_size const s3 = HIGH) {
        level[0] = s0; level[1] = s1;
        level[2] = s2; level[3] = s3;
        static_assert(size == 4, "");
        static_assert(HIGH_HIGH == 1 + spatial_cell::id_type(-1), "");
    }
    int operator[](size_t i) const {
        SDL_ASSERT(i < size);
        SDL_ASSERT(!(level[i] % 4));
        return level[i];
    }
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

using point_2D = point_XY<double>;
using point_3D = point_XYZ<double>;

struct spatial_rect {
    double min_lat;
    double min_lon;
    double max_lat;
    double max_lon;
    bool is_valid() const {
        SDL_ASSERT(min_lat < max_lat);
        SDL_ASSERT(min_lon != max_lon);
        return
            spatial_point::valid_latitude(min_lat) &&
            spatial_point::valid_longitude(min_lon) &&
            spatial_point::valid_latitude(max_lat) &&
            spatial_point::valid_longitude(max_lon);
    }
    static const size_t size = 4;
    spatial_point operator[](size_t const i) const; // counter-clock wize
};

struct polar_2D {
    double radial;
    double arg; // in radians
    static polar_2D polar(point_2D const &);
};

struct bound_boox {
    point_2D lt, rb;
};

#pragma pack(pop)

using vector_cell = std::vector<spatial_cell>;

using SP = spatial_point;
using XY = point_XY<int>;

} // db
} // sdl

#include "spatial_type.inl"

#endif // __SDL_SYSTEM_SPATIAL_TYPE_H__