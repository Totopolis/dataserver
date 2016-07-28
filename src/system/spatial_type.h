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
    uint32 id32() const {
        static_assert(sizeof(uint32) == sizeof(data.id), "");
        return reinterpret_cast<uint32 const &>(data.id);
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

#if defined(SDL_VISUAL_STUDIO_2013)
    static const double min_latitude;
    static const double max_latitude;
    static const double min_longitude;
    static const double max_longitude;
#else
    static constexpr double min_latitude    = -90;
    static constexpr double max_latitude    = 90;
    static constexpr double min_longitude   = -180;
    static constexpr double max_longitude   = 180;
#endif
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
    static double norm_longitude(double); // wrap around meridian +/-180
    static double norm_latitude(double); // wrap around poles +/-90
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

#if SDL_DEBUG
#define high_grid_optimization   0
#else
#define high_grid_optimization   1
#endif

#if high_grid_optimization
struct spatial_grid {
    enum grid_size : uint8 {
        LOW     = 4,    // 4X4,     16 cells
        MEDIUM  = 8,    // 8x8,     64 cells
        HIGH    = 16    // 16x16,   256 cells
    };
    enum { HIGH_HIGH = HIGH * HIGH };
    static const size_t size = spatial_cell::size;
    int operator[](size_t i) const {
        SDL_ASSERT(i < size);
        return HIGH;
    }
    static constexpr double f_0() { return 1.0 / HIGH; }
    static constexpr double f_1() { return f_0() / HIGH; }
    static constexpr double f_2() { return f_1() / HIGH; }
    static constexpr double f_3() { return f_2() / HIGH; }

    static constexpr int s_0() { return HIGH; }
    static constexpr int s_1() { return HIGH * s_0(); }
    static constexpr int s_2() { return HIGH * s_1(); }
    static constexpr int s_3() { return HIGH * s_2(); }
};
#else
struct spatial_grid { // 4 bytes
    enum grid_size : uint8 {
        LOW     = 4,    // 4X4,     16 cells
        MEDIUM  = 8,    // 8x8,     64 cells
        HIGH    = 16    // 16x16,   256 cells
    };
    enum { HIGH_HIGH = HIGH * HIGH };
    static const size_t size = spatial_cell::size;
    grid_size level[size];
    
    spatial_grid() {
        level[0] = HIGH;
        level[1] = HIGH;
        level[2] = HIGH;
        level[3] = HIGH;
    }
    explicit spatial_grid(
        grid_size const s0,
        grid_size const s1 = HIGH,
        grid_size const s2 = HIGH,
        grid_size const s3 = HIGH) {
        level[0] = s0; level[1] = s1;
        level[2] = s2; level[3] = s3;
        static_assert(size == 4, "");
        static_assert(HIGH_HIGH == 1 + spatial_cell::id_type(-1), "");
        static_assert(HIGH_HIGH * HIGH_HIGH == 1 + uint16(-1), "");
    }
    int operator[](size_t i) const {
        SDL_ASSERT(i < size);
        SDL_ASSERT(!(level[i] % 4));
        return level[i];
    }
    double f_0() const { return 1.0 / level[0]; }
    double f_1() const { return f_0() / level[1]; }
    double f_2() const { return f_1() / level[2]; }
    double f_3() const { return f_2() / level[3]; }

    int s_0() const { return level[0]; }
    int s_1() const { return level[1] * s_0(); }
    int s_2() const { return level[2] * s_1(); }
    int s_3() const { return level[3] * s_2(); }
};
#endif

template<typename T, bool>
struct swap_point_XY {
    using type = T;
    type X, Y;
};

template<typename T>
struct swap_point_XY<T, true> {
    using type = T;
    type Y, X;
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

template<bool flag>
using swap_point_2D = swap_point_XY<double, flag>;

using point_2D = point_XY<double>;
using point_3D = point_XYZ<double>;

template<typename point>
struct rect_t {
    point lt; // left top
    point rb; // right bottom
    point lb() const { return { lt.X, rb.Y }; } // left bottom
    point rt() const { return { rb.X, lt.Y }; } // right top
};

using rect_2D = rect_t<point_2D>;

struct spatial_rect {
    double min_lat;
    double min_lon;
    double max_lat;
    double max_lon;

    static const size_t size = 4;
    spatial_point operator[](size_t const i) const; // counter-clock wize
    spatial_point min() const;
    spatial_point max() const;

    bool is_null() const;
    bool cross_equator() const;
    explicit operator bool() const {
        return !is_null();
    }
    bool is_valid() const;    
    bool equal(spatial_rect const &) const;

    static spatial_rect init(spatial_point const &, spatial_point const &);
    static spatial_rect init(Latitude, Longitude, Latitude, Longitude);
};

struct polar_2D {
    double radial;
    double arg; // in radians
    static polar_2D polar(point_2D const &);
};

#pragma pack(pop)

using vector_cell = std::vector<spatial_cell>;
using vector_point_2D = std::vector<point_2D>;

using SP = spatial_point;
using XY = point_XY<int>;

} // db
} // sdl

#include "spatial_type.inl"

#endif // __SDL_SYSTEM_SPATIAL_TYPE_H__