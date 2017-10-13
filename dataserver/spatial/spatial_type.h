// spatial_type.h
//
#pragma once
#ifndef __SDL_SPATIAL_SPATIAL_TYPE_H__
#define __SDL_SPATIAL_SPATIAL_TYPE_H__

#include "dataserver/system/page_type.h"
#include "dataserver/common/array_enum.h"

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
    enum { allow_default_ctor = true };
    enum { allow_increment = true };
    enum { allow_decrement = true };
};

template<> struct quantity_traits<db::Degree> {
    enum { allow_default_ctor = true };
    enum { allow_increment = true };
    enum { allow_decrement = true };
};

template<> struct quantity_traits<db::Radian> {
    enum { allow_default_ctor = true };
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
    //FIXME: collection,
    _end
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
    template<size_t i>
    id_type get_id() const {
        static_assert(i < size, "");
        return data.id.cell[i];
    }
    template<size_t i> 
    void set_zero() {
        static_assert(i < size, "");
        data.id.cell[i] = 0;
    }
    template<size_t i, typename value_type> 
    void set_id(value_type id) {
        A_STATIC_ASSERT_TYPE(id_type, value_type);
        static_assert(i < size, "");
        data.id.cell[i] = id;
    }
    id_type operator[](size_t i) const {
        SDL_ASSERT(i < size);
        return data.id.cell[i];
    }
    id_type & operator[](size_t i) {
        SDL_ASSERT(i < size);
        return data.id.cell[i];
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

template<spatial_cell::depth_t T> 
using cell_depth_t = Val2Type<spatial_cell::depth_t, T>;

struct spatial_point { // 16 bytes

    double latitude;
    double longitude;

    static constexpr double min_latitude    = -90;
    static constexpr double max_latitude    = 90;
    static constexpr double min_longitude   = -180;
    static constexpr double max_longitude   = 180;

    static constexpr bool valid_latitude(double const d) {
        return frange(d, min_latitude, max_latitude);
    }
    static constexpr bool valid_longitude(double const d) {
        return frange(d, min_longitude, max_longitude);
    }
    static constexpr bool is_valid(Latitude const d) {
        return valid_latitude(d.value());
    }
    static constexpr bool is_valid(Longitude const d) {
        return valid_longitude(d.value());
    }
    bool is_valid() const {
        return is_valid(Latitude(this->latitude)) && is_valid(Longitude(this->longitude));
    }
    static double norm_longitude(double); // wrap around meridian +/-180
    static double norm_latitude(double); // wrap around poles +/-90    
    static spatial_point normalize(spatial_point const & p) {
        return { 
            norm_latitude(p.latitude), 
            norm_longitude(p.longitude)
        };
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
    bool equal_zero() const {
        return fzero(latitude) && fzero(longitude); 
    }
    bool match(spatial_point const &) const;
    static bool match(spatial_point const & p1, spatial_point const & p2) {
        return p1.match(p2);
    }
    static spatial_point STPointFromText(const std::string &); // POINT (longitude latitude)
};

#if SDL_DEBUG
inline std::ostream & operator <<(std::ostream & out, spatial_point const & p) {
    out << p.latitude << "," << p.longitude;
    return out;
}
#endif

template<bool high_grid> struct spatial_grid_high;
template<> struct spatial_grid_high<true> {
    static constexpr bool is_high_grid = true;
    enum grid_size : uint8 {
        LOW     = 4,    // 4X4,     16 cells
        MEDIUM  = 8,    // 8x8,     64 cells
        HIGH    = 16    // 16x16,   256 cells
    };
    enum { HIGH_HIGH = HIGH * HIGH };
    spatial_grid_high(){}
    static constexpr size_t size = spatial_cell::size; // = 4
    template<size_t i> static constexpr int get() {
        static_assert(i < size, "");
        return HIGH;
    }
    int operator[](size_t i) const {
        SDL_ASSERT(i < size);
        return HIGH;
    }
    static constexpr double f_0() { return 1.0 / HIGH; }    // 0.0625
    static constexpr double f_1() { return f_0() / HIGH; }  // 0.00390625
    static constexpr double f_2() { return f_1() / HIGH; }  // 0.000244140625
    static constexpr double f_3() { return f_2() / HIGH; }  // 0.0000152587890625

    //FIXME: consider quantity<>

    // top down
    static constexpr int s_0() { return HIGH; }         // 16
    static constexpr int s_1() { return HIGH * s_0(); } // 256
    static constexpr int s_2() { return HIGH * s_1(); } // 4096
    static constexpr int s_3() { return HIGH * s_2(); } // 65536

    // bottom up
    static constexpr int b_3() { return HIGH; }         // 16
    static constexpr int b_2() { return HIGH * b_3(); } // 256
    static constexpr int b_1() { return HIGH * b_2(); } // 4096
    static constexpr int b_0() { return HIGH * b_1(); } // 65536
};

template<> struct spatial_grid_high<false> { // 4 bytes
    static constexpr bool is_high_grid = false;
    enum grid_size : uint8 {
        LOW     = 4,    // 4X4,     16 cells
        MEDIUM  = 8,    // 8x8,     64 cells
        HIGH    = 16    // 16x16,   256 cells
    };
    enum { HIGH_HIGH = HIGH * HIGH };
    static constexpr size_t size = spatial_cell::size; // = 4
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
    template<size_t i> int get() const {
        static_assert(i < size, "");
        return level[i];
    }
    int operator[](size_t const i) const {
        SDL_ASSERT(i < size);
        return level[i];
    }
    double f_0() const { return 1.0 / level[0]; }
    double f_1() const { return f_0() / level[1]; }
    double f_2() const { return f_1() / level[2]; }
    double f_3() const { return f_2() / level[3]; }

    //FIXME: consider quantity<>

    // top down
    int s_0() const { return level[0]; }
    int s_1() const { return level[1] * s_0(); }
    int s_2() const { return level[2] * s_1(); }
    int s_3() const { return level[3] * s_2(); }

    // bottom up
    int b_3() const { return level[3]; }
    int b_2() const { return level[2] * b_3(); }
    int b_1() const { return level[1] * b_2(); }
    int b_0() const { return level[0] * b_1(); }
};

// #if high_grid_optimization should be replaced by templates
#define high_grid_optimization   1
using spatial_grid = spatial_grid_high<high_grid_optimization>;
using is_high_grid = bool_constant<spatial_grid::is_high_grid>;

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
    type left() const { return lt.X; }
    type top() const { return lt.Y; }
    type right() const { return rb.X; }
    type bottom() const { return rb.Y; }
    bool is_valid() const {
        return (lt.X <= rb.X)
            && (lt.Y <= rb.Y);
    }
};

struct spatial_rect {
    double min_lat;
    double min_lon;
    double max_lat;
    double max_lon;

    static constexpr size_t size = 4;
    spatial_point operator[](size_t) const; // counter-clock wize
    spatial_point min_pt() const;
    spatial_point max_pt() const;
    spatial_point center() const;

    bool is_null() const; //FIXME: min_lon = -180, max_lon = +180 ?
    bool cross_equator() const;
    explicit operator bool() const {
        return !is_null();
    }
    bool is_valid() const;    
    bool equal(spatial_rect const &) const;
    bool is_inside(spatial_point const &) const;

    static spatial_rect init(spatial_point const &, spatial_point const &);
    static spatial_rect init(Latitude, Longitude, Latitude, Longitude);
    static spatial_rect normalize(spatial_rect);
};

#if SDL_DEBUG
inline std::ostream & operator <<(std::ostream & out, spatial_rect const & rc) {
    out << rc.min_lat << ","
        << rc.min_lon << ","
        << rc.max_lat << ","
        << rc.max_lon;
    return out;
}
#endif

struct polar_2D {
    double radial;
    double arg; // in radians
    static polar_2D polar(point_2D const & s) {
        polar_2D p;
        p.radial = std::sqrt(s.X * s.X + s.Y * s.Y);
        p.arg = fatan2(s.Y, s.X);
        return p;
    }
    static double polar_arg(point_2D const & s) {
        return fatan2(s.Y, s.X);
    }
    static double polar_arg(double X, double Y) {
        return fatan2(Y, X);
    }
};

template<typename T, T _scale>
struct spatial_point_int_t {
    static constexpr T scale = _scale;
    using type = T;
    type latitude;
    type longitude;
    void assign(spatial_point const & src) noexcept {
        latitude = static_cast<type>(scale * src.latitude);
        longitude = static_cast<type>(scale * src.longitude); 
    }
    static spatial_point_int_t make(spatial_point const & src) noexcept {
        spatial_point_int_t p;
        p.assign(src);
        return p;
    }
    spatial_point cast_double() const noexcept {
        spatial_point p;
        p.latitude = 1.0 * this->latitude / scale;
        p.longitude = 1.0 * this->longitude / scale;
        return p;
    }
};

template<typename T, T scale>
inline bool operator < (spatial_point_int_t<T, scale> const & x,
                        spatial_point_int_t<T, scale> const & y) { 
    if (x.longitude < y.longitude) return true;
    if (y.longitude < x.longitude) return false;
    return x.latitude < y.latitude;
}

template<typename T, T _scale>
struct spatial_rect_int_t {
    static constexpr T scale = _scale;
    using type = T;
    type min_lat;
    type min_lon;
    type max_lat;
    type max_lon;
    void assign(spatial_rect const & src) noexcept {
        min_lat = static_cast<type>(scale * src.min_lat);
        min_lon = static_cast<type>(scale * src.min_lon);
        max_lat = static_cast<type>(scale * src.max_lat);
        max_lon = static_cast<type>(scale * src.max_lon);
    }
    static spatial_rect_int_t make(spatial_rect const & src) noexcept {
        spatial_rect_int_t p;
        p.assign(src);
        return p;
    }
    spatial_rect cast_double() const noexcept {
        spatial_rect p;
        p.min_lat = 1.0 * this->min_lat / scale;
        p.min_lon = 1.0 * this->min_lon / scale;
        p.max_lat = 1.0 * this->max_lat / scale;
        p.max_lon = 1.0 * this->max_lon / scale;
        return p;
    }
};

template<typename T, T scale>
inline bool operator < (spatial_rect_int_t<T, scale> const & a,
                        spatial_rect_int_t<T, scale> const & b) { 
    if (a.min_lat < b.min_lat) return true;
    if (b.min_lat < a.min_lat) return false;
    if (a.min_lon < b.min_lon) return true;
    if (b.min_lon < a.min_lon) return false;
    if (a.max_lat < b.max_lat) return true;
    if (b.max_lat < a.max_lat) return false;
    return (a.max_lon < b.max_lon);
}

using spatial_point_int32 = spatial_point_int_t <int32, 10000000>;
using spatial_rect_int32 = spatial_rect_int_t   <int32, 10000000>;

#pragma pack(pop)

using SP = spatial_point;
using XY = point_XY<int>;

using rect_2D = rect_t<point_2D>;
using rect_XY = rect_t<XY>;

inline size_t rect_width(rect_XY const & bbox) {
    SDL_ASSERT(bbox.lt.X <= bbox.rb.X);
    return bbox.rb.X - bbox.lt.X + 1;
}

inline size_t rect_height(rect_XY const & bbox) {
    SDL_ASSERT(bbox.lt.Y <= bbox.rb.Y);
    return bbox.rb.Y - bbox.lt.Y + 1;
}

inline size_t rect_area(rect_XY const & bbox) {
    return rect_width(bbox) * rect_height(bbox);
}

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

inline constexpr bool is_exterior(orientation t) { return orientation::exterior == t; }
inline constexpr bool is_interior(orientation t) { return orientation::interior == t; }

inline constexpr bool is_counterclockwise(winding t) { return winding::counterclockwise == t; }
inline constexpr bool is_clockwise(winding t) { return winding::clockwise == t; }

enum class intersect_flag {
    linestring,
    polygon,
    multipoint
};

template<sortorder ord> struct quantity_less;

template<> struct quantity_less<sortorder::ASC> {
    template<class arg_type>
    static constexpr bool less(arg_type const & x, arg_type const & y) {
        return x.value() < y.value();
    }
};

template<> struct quantity_less<sortorder::DESC> {
    template<class arg_type>
    static constexpr bool less(arg_type const & x, arg_type const & y) {
        return y.value() < x.value();
    }
};

template<sortorder ord> struct value_less;

template<> struct value_less<sortorder::ASC> {
    template<class arg_type>
    static constexpr bool less(arg_type const & x, arg_type const & y) {
        return x < y;
    }
};

template<> struct value_less<sortorder::DESC> {
    template<class arg_type>
    static constexpr bool less(arg_type const & x, arg_type const & y) {
        return y < x;
    }
};

template<class T>
using array_spatial_type_t = array_enum_t<T, (int)spatial_type::_end, spatial_type>;

} // db
} // sdl

#include "dataserver/spatial/spatial_type.inl"

#endif // __SDL_SPATIAL_SPATIAL_TYPE_H__