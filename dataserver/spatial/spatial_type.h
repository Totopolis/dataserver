// spatial_type.h
//
#pragma once
#ifndef __SDL_SYSTEM_SPATIAL_TYPE_H__
#define __SDL_SYSTEM_SPATIAL_TYPE_H__

#include "system/page_type.h"

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

} // db

template<> struct quantity_traits<db::Meters> {
    enum { allow_increment = true };
    enum { allow_decrement = true };
};

namespace db {

enum class spatial_type {
    null = 0,
    point,
    linestring,
    polygon,            // ring_num == 1
    linesegment,
    multilinestring,    // numobj > 1
    multipolygon,       // numobj > 1, ring_num > 1, use ring_orient()
    //FIXME: multipoint,
};

// OGC compatible types
enum class geometry_types : uint8 { // https://en.wikipedia.org/wiki/Well-known_text
    Unknown = 0,
    Point = 1,              // POINT
    LineString = 2,         // LINESTRING
    Polygon = 3,            // POLYGON
    MultiPoint = 4,         // MULTIPOINT
    MultiLineString = 5,    // MULTILINESTRING 
    MultiPolygon = 6,       // MULTIPOLYGON 
    GeometryCollection = 7, // GEOMETRYCOLLECTION
};

#pragma pack(push, 1) 

struct spatial_tag { // 2 bytes, TYPEID
    enum type {
        t_none = 0,
        t_point = 0x0C01,             // 3073
        t_multipolygon = 0x0401,      // 1025
        t_linesegment = 0x1401,       // 5121
        t_linestring = 2,
    };
    uint16  _16; 
    operator type() const {
        return static_cast<type>(_16);
    }
};

#define spatial_cell_optimization   1
struct spatial_cell { // 5 bytes
    enum depth_t : uint8 {
        depth_1 = 1,
        depth_2,
        depth_3,
        depth_4,
    };
    static constexpr size_t size = depth_4; // max depth
    using id_type = uint8;
    union id_array { // 4 bytes
        id_type cell[size];
        uint32 _32;
        uint32 r32() const {
            return reverse_bytes(_32);
        }
    };
    struct data_type { // 5 bytes
        id_array id;
        id_type depth; // 1..4
    };
    union {
        data_type data;
        char raw[sizeof(data_type)];
    };
    uint32 r32() const {
        return data.id.r32();
    }
    id_type operator[](size_t i) const {
        SDL_ASSERT(i < size);
        return data.id.cell[i];
    }
    id_type & operator[](size_t i) {
        SDL_ASSERT(i < size);
        return data.id.cell[i];
    }
    template<size_t i>
    void set_id(id_type id) {
        static_assert(i < size, "");
        data.id.cell[i] = id;
    }
    bool is_null() const {
        SDL_ASSERT(data.depth <= size);
        return 0 == data.depth;
    }
    explicit operator bool() const {
        return !is_null();
    }
	void set_depth(id_type); // makes zero tail
    static spatial_cell init(uint32);
    static spatial_cell init(uint32, id_type depth);
	static spatial_cell init(spatial_cell, id_type depth);
    static spatial_cell set_depth(spatial_cell, id_type);
    static spatial_cell min();
    static spatial_cell max();
    static spatial_cell parse_hex(const char *);
    bool intersect(spatial_cell const &) const;
    static bool less(spatial_cell const &, spatial_cell const &);
    static bool equal(spatial_cell const &, spatial_cell const &);
    bool zero_tail() const;
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

template<bool high_grid> struct spatial_grid_high;
template<> struct spatial_grid_high<true> {
    enum grid_size : uint8 {
        LOW     = 4,    // 4X4,     16 cells
        MEDIUM  = 8,    // 8x8,     64 cells
        HIGH    = 16    // 16x16,   256 cells
    };
    enum { HIGH_HIGH = HIGH * HIGH };
    spatial_grid_high(){}
    static constexpr size_t size = spatial_cell::size;
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

template<> struct spatial_grid_high<false> { // 4 bytes
    enum grid_size : uint8 {
        LOW     = 4,    // 4X4,     16 cells
        MEDIUM  = 8,    // 8x8,     64 cells
        HIGH    = 16    // 16x16,   256 cells
    };
    enum { HIGH_HIGH = HIGH * HIGH };
    static constexpr size_t size = spatial_cell::size;
    grid_size level[size];
    
    spatial_grid_high() {
        level[0] = HIGH;
        level[1] = HIGH;
        level[2] = HIGH;
        level[3] = HIGH;
    }
    explicit spatial_grid_high(
        grid_size const s0,
        grid_size const s1 = HIGH,
        grid_size const s2 = HIGH,
        grid_size const s3 = HIGH) {
        level[0] = s0; level[1] = s1;
        level[2] = s2; level[3] = s3;
        static_assert(size == 4, "");
        static_assert(HIGH_HIGH == 1 + uint8(-1), "");
        static_assert(HIGH_HIGH * HIGH_HIGH == 1 + uint16(-1), "");
    }
    int operator[](size_t const i) const {
        SDL_ASSERT(i < size);
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

#define high_grid_optimization   1
using spatial_grid = spatial_grid_high<high_grid_optimization>;

#if high_grid_optimization
template<spatial_cell::depth_t depth>
struct cell_capacity {
private:
    static_assert(depth != spatial_cell::depth_4, "");
    static constexpr spatial_cell::depth_t next_depth = (spatial_cell::depth_t)((uint8)depth + 1);
public:
    static constexpr uint32 grid = spatial_grid::grid_size::HIGH * cell_capacity<next_depth>::grid;
    static constexpr uint32 value = grid * grid;
    static constexpr uint32 upper = uint32(value - 1);
    static constexpr uint32 step = cell_capacity<next_depth>::value;
};
template<> struct cell_capacity<spatial_cell::depth_4> {
    static constexpr uint32 grid = spatial_grid::grid_size::HIGH; // = 16
    static constexpr uint32 value = grid * grid; // = 256
    static constexpr uint32 upper = uint32(value - 1);
    static constexpr uint32 step = 1;
};
template<> struct cell_capacity<spatial_cell::depth_1> {
    static constexpr uint32 grid = spatial_grid::grid_size::HIGH * cell_capacity<spatial_cell::depth_2>::grid;
    static constexpr uint64 value64 = uint64(grid) * grid; // uint64 to avoid overflow
    static constexpr uint32 upper = uint32(value64 - 1);
    static constexpr uint32 step = cell_capacity<spatial_cell::depth_2>::value;
};
#endif // high_grid_optimization

template<typename T, bool>
struct swap_point {
    using type = T;
    type X, Y;
};

template<typename T>
struct swap_point<T, true> {
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
using swap_point_2D = swap_point<double, flag>;

using point_2D = point_XY<double>;
using point_3D = point_XYZ<double>;

template<typename point>
struct rect_t {
    using type = typename point::type;
    point lt; // left top
    point rb; // right bottom
    point lb() const { return { lt.X, rb.Y }; } // left bottom
    point rt() const { return { rb.X, lt.Y }; } // right top
    type width() const {
        SDL_ASSERT(lt.X <= rb.X); // warning
        return rb.X - lt.X;
    }
    type height() const {
        SDL_ASSERT(lt.Y <= rb.Y); // warning
        return rb.Y - lt.Y;
    }
    type left() const { return lt.X; }
    type top() const { return lt.Y; }
    type right() const { return rb.X; }
    type bottom() const { return rb.Y; }
};

struct spatial_rect {
    double min_lat;
    double min_lon;
    double max_lat;
    double max_lon;

    static constexpr size_t size = 4;
    spatial_point operator[](size_t) const; // counter-clock wize
    spatial_point min() const;
    spatial_point max() const;
    spatial_point center() const;

    bool is_null() const;
    bool cross_equator() const;
    explicit operator bool() const {
        return !is_null();
    }
    bool is_valid() const;    
    bool equal(spatial_rect const &) const;
    bool is_inside(spatial_point const &) const;

    static spatial_rect init(spatial_point const &, spatial_point const &);
    static spatial_rect init(Latitude, Longitude, Latitude, Longitude);
};

struct polar_2D {
    double radial;
    double arg; // in radians
    static polar_2D polar(point_2D const &);
};

#pragma pack(pop)

using SP = spatial_point;
using XY = point_XY<int>;

using rect_2D = rect_t<point_2D>;
using rect_XY = rect_t<XY>;

using vector_cell = std::vector<spatial_cell>;
using vector_point_2D = std::vector<point_2D>;
using vector_XY = std::vector<XY>;

enum class orientation : uint8 {
    exterior = 0,
    interior,
};

enum class winding : uint8 {
    counterclockwise = 0,
    clockwise,
    exterior = counterclockwise,
    interior = clockwise 
};

inline bool is_exterior(orientation t) { return orientation::exterior == t; }
inline bool is_interior(orientation t) { return orientation::interior == t; }

inline bool is_counterclockwise(winding t) { return winding::counterclockwise == t; }
inline bool is_clockwise(winding t) { return winding::clockwise == t; }

enum class intersect_type {
    linestring,
    polygon
};

template<intersect_type T> 
using intersect_t = Val2Type<intersect_type, T>;

} // db
} // sdl

#include "spatial_type.inl"

#endif // __SDL_SYSTEM_SPATIAL_TYPE_H__