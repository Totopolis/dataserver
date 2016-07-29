// transform.cpp
//
#include "common/common.h"
#include "common/static_type.h"
#include "transform.h"
#include "hilbert.inl"
#include "transform.inl"
#include <algorithm>
#include <set>

namespace sdl { namespace db {

class interval_cell: noncopyable {
public:
    struct compare {
        static bool less(spatial_cell const & x, spatial_cell const & y) {
            SDL_ASSERT(x.zero_tail());
            SDL_ASSERT(y.zero_tail());
            return x.data.id.r32() < y.data.id.r32();
        }
        bool operator () (spatial_cell const & x, spatial_cell const & y) const {
            return compare::less(x, y);
        }
    };
private:
    using set_type = std::set<spatial_cell, compare>;
    set_type m_set;
public:
    interval_cell() = default;
    bool insert(spatial_cell const &);
};

bool interval_cell::insert(spatial_cell const & val) {
    SDL_ASSERT(val.depth() == 4);
    auto right = m_set.lower_bound(val);
    if (right != m_set.end()) {
        if (right->data.id._32 == val.data.id._32) {
            return false; // already exists
        }
        SDL_ASSERT(compare::less(val, *right));
#if 0
        if (right != m_set.begin()) {
            auto left = right; --left;
            SDL_ASSERT(compare::less(*left, val));
            if (left->data.depth == 0) {
                return false; // val is inside interval [left..right]
            }
        }
        else {
            if (reverse_bytes(val.id32()) + 1 == reverse_bytes(right->id32())) {}
        }
#endif
    }
    return m_set.insert(val).second;
}


namespace space { 

using pair_size_t = std::pair<size_t, size_t>;

template<class iterator, class fun_type>
pair_size_t find_range(iterator first, iterator last, fun_type less) {
    SDL_ASSERT(first != last);
    auto p1 = first;
    auto p2 = p1;
    auto it = first; ++it;
    for  (; it != last; ++it) {
        if (less(*it, *p1)) {
            p1 = it;
        }
        else if (less(*p2, *it)) {
            p2 = it;
        }
    }
    return { p1 - first, p2 - first };
}

template<class iter_type>
rect_2D bound_box(iter_type first, iter_type end) {
    SDL_ASSERT(first != end);
    rect_2D rc;
    rc.lt = rc.rb = *(first++);
    for (; first != end; ++first) {
        auto const & p = *first;
        set_min(rc.lt.X, p.X);
        set_min(rc.lt.Y, p.Y);
        set_max(rc.rb.X, p.X);
        set_max(rc.rb.Y, p.Y);
    }
    SDL_ASSERT(rc.lt.X <= rc.rb.X);
    SDL_ASSERT(rc.lt.Y <= rc.rb.Y);
    return rc;
}

template<class vector_type>
vector_type sort_unique(vector_type && result) {
    if (!result.empty()) {
        std::sort(result.begin(), result.end());
        result.erase(std::unique(result.begin(), result.end()), result.end());
    }
    return std::move(result); // must return copy
}

template<class vector_type>
vector_type sort_unique(vector_type && v1, vector_type && v2) {
    if (v1.empty()) {
        return sort_unique(std::move(v2));
    }
    if (v2.empty()) {
        return sort_unique(std::move(v1));
    }
    v1.insert(v1.end(), v2.begin(), v2.end());
    return sort_unique(std::move(v1));
}

#if 0
template<class vector_type> inline
vector_type & insert_end(vector_type & dest, vector_type && src) {
    SDL_ASSERT(&dest != &src);
    dest.insert(dest.end(), src.begin(), src.end());
    return dest;
}
#endif
//------------------------------------------------------------------------

struct math : is_static {
    enum { EARTH_ELLIPSOUD = 0 }; // to be tested
    enum quadrant {
        q_0 = 0, // [-45..45) longitude
        q_1 = 1, // [45..135)
        q_2 = 2, // [135..180][-180..-135)
        q_3 = 3  // [-135..-45)
    };
    enum { quadrant_size = 4 };
    static const double sorted_quadrant[quadrant_size]; // longitudes
    enum class hemisphere {
        north = 0, // [0..90] latitude
        south = 1  // [-90..0)
    };
    struct sector_t {
        hemisphere h;
        quadrant q;
    };
    using sector_index = std::pair<sector_t, size_t>;
    using sector_indexes = std::vector<sector_index>;

    static hemisphere latitude_hemisphere(double const lat) {
        return (lat >= 0) ? hemisphere::north : hemisphere::south;
    }
    static hemisphere latitude_hemisphere(spatial_point const & s) {
        return latitude_hemisphere(s.latitude);
    }
    static hemisphere point_hemisphere(point_2D const & p) {
        return (p.Y >= 0.5) ? hemisphere::north : hemisphere::south;
    }
    static bool latitude_pole(double const lat) {
        return fequal(lat, (lat > 0) ? 90 : -90);
    }
    static sector_t spatial_sector(spatial_point const &);
    static quadrant longitude_quadrant(double);
    static quadrant longitude_quadrant(Longitude);
    static bool cross_longitude(double mid, double left, double right);
    static double longitude_distance(double left, double right);
    static double longitude_distance(double left, double right, bool);
    static bool rect_cross_quadrant(spatial_rect const &);
    static double longitude_meridian(double, quadrant);
    static double reverse_longitude_meridian(double, quadrant);
    static point_3D cartesian(Latitude, Longitude);
    static spatial_point reverse_cartesian(point_3D const &);
    static point_3D line_plane_intersect(Latitude, Longitude);
    static point_2D scale_plane_intersect(const point_3D &, quadrant, hemisphere);
    static point_2D project_globe(spatial_point const &);
    static point_2D project_globe(spatial_point const &, hemisphere);
    static point_2D project_globe(Latitude const lat, Longitude const lon);
    static spatial_cell globe_to_cell(const point_2D &, spatial_grid);
    static double norm_longitude(double);
    static double norm_latitude(double);
    static double add_longitude(double const lon, double const d);
    static double add_latitude(double const lat, double const d);
    static Meters haversine(spatial_point const & p1, spatial_point const & p2, Meters R);
    static Meters haversine(spatial_point const & p1, spatial_point const & p2);
    static spatial_point destination(spatial_point const &, Meters const distance, Degree const bearing);
    static point_XY<int> quadrant_grid(quadrant, int const grid);
    static point_XY<int> multiply_grid(point_XY<int> const & p, int const grid);
    static quadrant point_quadrant(point_2D const & p);
    static spatial_point reverse_line_plane_intersect(point_3D const &);
    static point_3D reverse_scale_plane_intersect(point_2D const &, quadrant, hemisphere);
    static spatial_point reverse_project_globe(point_2D const &);
    static bool destination_rect(spatial_rect &, spatial_point const &, Meters);
    static void polygon_latitude(vector_point_2D &, double const lat, double const lon1, double const lon2, hemisphere, bool);
    static void polygon_contour(vector_point_2D &, spatial_rect const &, hemisphere);
    static sector_indexes polygon_range(vector_point_2D &, spatial_point const &, Meters, sector_t const &);
    static void vertical_fill(vector_cell &, point_2D const *, point_2D const *, spatial_grid, vector_point_2D const * = nullptr);
    static void horizontal_fill(vector_cell &, point_2D const *, point_2D const *, spatial_grid, vector_point_2D const * = nullptr);
    static spatial_cell make_cell(XY const &, spatial_grid);
    static vector_cell select_hemisphere(spatial_rect const &, spatial_grid);
    static void select_sector(vector_cell &, spatial_rect const &, spatial_grid);    
    static vector_cell select_range(spatial_point const &, Meters, spatial_grid);
    static void select_range_sector(vector_cell &, spatial_point const &, Meters, spatial_grid,
        point_2D const *, 
        point_2D const *,
        vector_point_2D const * = nullptr);
private:
    using ellipsoid_true = bool_constant<true>;
    using ellipsoid_false = bool_constant<false>;
    static double earth_radius(double, ellipsoid_true);
    static double earth_radius(double, double, ellipsoid_true);
    static double earth_radius(double, ellipsoid_false) {
        return limits::EARTH_RADIUS;
    }
    static double earth_radius(double, double, ellipsoid_false) {
        return limits::EARTH_RADIUS;
    }
public:
    static double earth_radius(Latitude const lat) {
        return earth_radius(lat.value(), bool_constant<EARTH_ELLIPSOUD>());
    }
    static double earth_radius(Latitude const lat1, Latitude const lat2) {
        return earth_radius(lat1.value(), lat2.value(), bool_constant<EARTH_ELLIPSOUD>());
    }
    static double earth_radius(spatial_point const & p) {
        return earth_radius(p.latitude);
    }
    static size_t remain(size_t const x, size_t const y) {
        size_t const d = x % y;
        return d ? (y - d) : 0;
    }
    static size_t roundup(double const x, size_t const y) {
        SDL_ASSERT(x >= 0);
        size_t const d = a_max(static_cast<size_t>(x + 0.5), y); 
        return d + remain(d, y); 
    }
};

const double math::sorted_quadrant[quadrant_size] = { -135, -45, 45, 135 };

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

math::quadrant math::longitude_quadrant(double const x) {
    SDL_ASSERT(SP::valid_longitude(x));
    if (x >= 0) {
        if (x < 45) return q_0;
        if (x < 135) return q_1;
    }
    else {
        if (x >= -45) return q_0;
        if (x >= -135) return q_3;
    }
    return q_2; 
}

inline math::quadrant math::longitude_quadrant(Longitude const x) {
    return longitude_quadrant(x.value());
}

inline math::sector_t math::spatial_sector(spatial_point const & p) {
    return {  
        latitude_hemisphere(p.latitude),
        longitude_quadrant(p.longitude) };
}

point_3D math::cartesian(Latitude const lat, Longitude const lon) { // 3D point on the globe
    SDL_ASSERT(spatial_point::is_valid(lat));
    SDL_ASSERT(spatial_point::is_valid(lon));
    double const L = std::cos(lat.value() * limits::DEG_TO_RAD);
    point_3D p;
    p.X = L * std::cos(lon.value() * limits::DEG_TO_RAD);
    p.Y = L * std::sin(lon.value() * limits::DEG_TO_RAD);
    p.Z = std::sin(lat.value() * limits::DEG_TO_RAD);
    return p;
}

spatial_point math::reverse_cartesian(point_3D const & p) { // p = 3D point on the globe
    SDL_ASSERT(fequal(length(p), 1));
    spatial_point s;
    if (p.Z >= 1.0 - limits::fepsilon)
        s.latitude = 90; 
    else if (p.Z <= -1.0 + limits::fepsilon)
        s.latitude = -90;
    else
        s.latitude = std::asin(p.Z) * limits::RAD_TO_DEG;
    s.longitude = fatan2(p.Y, p.X) * limits::RAD_TO_DEG;
    SDL_ASSERT(s.is_valid());
    return s;
}

namespace line_plane_intersect_ {
    static const point_3D P0 { 0, 0, 0 };
    static const point_3D V0 { 1, 0, 0 };
    static const point_3D e2 { 0, 1, 0 };
    static const point_3D e3 { 0, 0, 1 };
    static const point_3D N = normalize(point_3D{ 1, 1, 1 }); // plane P be given by a point V0 on it and a normal vector N
}

point_3D math::line_plane_intersect(Latitude const lat, Longitude const lon) { //http://geomalgorithms.com/a05-_intersect-1.html

    SDL_ASSERT(frange(lon.value(), 0, 90));
    SDL_ASSERT(frange(lat.value(), 0, 90));

    namespace use = line_plane_intersect_;
    const point_3D ray = cartesian(lat, lon); // cartesian position on globe
    const double n_u = scalar_mul(ray, use::N);
    
    SDL_ASSERT(fequal(length(ray), 1));
    SDL_ASSERT(n_u > 0);
    SDL_ASSERT(fequal(scalar_mul(use::N, use::N), 1.0));
    SDL_ASSERT(fequal(scalar_mul(use::N, use::V0), use::N.X)); // = N.X
    SDL_ASSERT(!point_on_plane(use::P0, use::V0, use::N));
    SDL_ASSERT(point_on_plane(use::e2, use::V0, use::N));
    SDL_ASSERT(point_on_plane(use::e3, use::V0, use::N));

    point_3D const p = multiply(ray, use::N.X / n_u); // distance = N * (V0 - P0) / n_u = N.X / n_u
    SDL_ASSERT(frange(p.X, 0, 1));
    SDL_ASSERT(frange(p.Y, 0, 1));
    SDL_ASSERT(frange(p.Z, 0, 1));
    SDL_ASSERT(p != use::P0);
    return p;
}

inline spatial_point math::reverse_line_plane_intersect(point_3D const & p)
{
    namespace use = line_plane_intersect_;
    SDL_ASSERT(frange(p.X, 0, 1));
    SDL_ASSERT(frange(p.Y, 0, 1));
    SDL_ASSERT(frange(p.Z, 0, 1));
    SDL_ASSERT(p != use::P0);
    return reverse_cartesian(normalize(p));
}

double math::longitude_meridian(double const x, const quadrant q) { // x = longitude
    SDL_ASSERT(a_abs(x) <= 180);
    if (x >= 0) {
        switch (q) {
        case q_0: return x + 45;
        case q_1: return x - 45;
        default:
            SDL_ASSERT(q == q_2);
            return x - 135;
        }
    }
    else {
        switch (q) {
        case q_0: return x + 45;
        case q_3: return x + 135;
        default:
            SDL_ASSERT(q == q_2);
            return x + 180 + 45;
        }
    }
}

double math::reverse_longitude_meridian(double const x, const quadrant q) { // x = longitude 
    SDL_ASSERT(frange(x, 0, 90));
    switch (q) {
    case q_0: return x - 45;
    case q_1: return x + 45;
    case q_2:
        if (x <= 45) {
            return x + 135;
        }
        return x - 180 - 45;
    default:
        SDL_ASSERT(q == q_3);
        return x - 135;
    }
}

namespace scale_plane_intersect_ {
    static const point_3D e1 { 1, 0, 0 };
    static const point_3D e2 { 0, 1, 0 };
    static const point_3D e3 { 0, 0, 1 };
    static const point_3D mid{ 0.5, 0.5, 0 };
    static const point_3D px = normalize(minus_point(e2, e1));
    static const point_3D py = normalize(minus_point(e3, mid));
    static const double lx = distance(e2, e1);
    static const double ly = distance(e3, mid);
    static const point_2D scale_02 { 0.5 / lx, 0.5 / ly };
    static const point_2D scale_13 { 1 / lx, 0.25 / ly };
}

point_2D math::scale_plane_intersect(const point_3D & p3, const quadrant quad, const hemisphere is_north)
{
    namespace use = scale_plane_intersect_;

    SDL_ASSERT_1(fequal(length(use::px), 1.0));
    SDL_ASSERT_1(fequal(length(use::py), 1.0));
    SDL_ASSERT_1(fequal(use::lx, std::sqrt(2.0)));
    SDL_ASSERT_1(fequal(use::ly, std::sqrt(1.5)));

    const point_3D v3 = minus_point(p3, use::e1);
    point_2D p2 = { 
        scalar_mul(v3, use::px), 
        scalar_mul(v3, use::py) };

    SDL_ASSERT_1(frange(p2.X, 0, use::lx));
    SDL_ASSERT_1(frange(p2.Y, 0, use::ly));

    if (quad & 1) { // 1, 3
        p2.X *= use::scale_13.X;
        p2.Y *= use::scale_13.Y;
        SDL_ASSERT_1(frange(p2.X, 0, 1));
        SDL_ASSERT_1(frange(p2.Y, 0, 0.25));
    }
    else { // 0, 2
        p2.X *= use::scale_02.X;
        p2.Y *= use::scale_02.Y;
        SDL_ASSERT_1(frange(p2.X, 0, 0.5));
        SDL_ASSERT_1(frange(p2.Y, 0, 0.5));
    }
    point_2D ret;
    if (hemisphere::north == is_north) {
        switch (quad) {
        case q_0:
            ret.X = 1 - p2.Y;
            ret.Y = 0.5 + p2.X;
            break;
        case q_1:
            ret.X = 1 - p2.X;
            ret.Y = 1 - p2.Y;
            break;
        case q_2:
            ret.X = p2.Y;
            ret.Y = 1 - p2.X;
            break;
        default:
            SDL_ASSERT(q_3 == quad);
            ret.X = p2.X;
            ret.Y = 0.5 + p2.Y;
            break;
        }
    }
    else {
        switch (quad) {
        case q_0:
            ret.X = 1 - p2.Y;
            ret.Y = 0.5 - p2.X;
            break;
        case q_1:
            ret.X = 1 - p2.X;
            ret.Y = p2.Y;
            break;
        case q_2:
            ret.X = p2.Y;
            ret.Y = p2.X;
            break;
        default:
            SDL_ASSERT(q_3 == quad);
            ret.X = p2.X;
            ret.Y = 0.5 - p2.Y;
            break;
        }
    }
    SDL_ASSERT_1(frange(ret.X, 0, 1));
    SDL_ASSERT_1(frange(ret.Y, 0, 1));
    return ret;
}

point_3D math::reverse_scale_plane_intersect(point_2D const & ret, quadrant const quad, const hemisphere is_north) 
{
    namespace use = scale_plane_intersect_;

    SDL_ASSERT_1(frange(ret.X, 0, 1));
    SDL_ASSERT_1(frange(ret.Y, 0, 1));

    //1) reverse scaling quadrant
    point_2D p2;
    if (hemisphere::north == is_north) {
        switch (quad) {
        case q_0:
            p2.Y = 1 - ret.X;   // ret.X = 1 - p2.Y;
            p2.X = ret.Y - 0.5; // ret.Y = 0.5 + p2.X;
            break;
        case q_1:
            p2.X = 1 - ret.X;   // ret.X = 1 - p2.X;
            p2.Y = 1 - ret.Y;   // ret.Y = 1 - p2.Y;
            break;
        case q_2:
            p2.Y = ret.X;       // ret.X = p2.Y;
            p2.X = 1 - ret.Y;   // ret.Y = 1 - p2.X;
            break;
        default:
            SDL_ASSERT(q_3 == quad);
            p2.X = ret.X;       // ret.X = p2.X;
            p2.Y = ret.Y - 0.5; // ret.Y = 0.5 + p2.Y;
            break;
        }
    }
    else {
        switch (quad) {
        case q_0:
            p2.Y = 1 - ret.X;   // ret.X = 1 - p2.Y;
            p2.X = 0.5 - ret.Y; // ret.Y = 0.5 - p2.X;
            break;
        case q_1:
            p2.X = 1 - ret.X;   // ret.X = 1 - p2.X;
            p2.Y = ret.Y;       // ret.Y = p2.Y;
            break;
        case q_2:
            p2.Y = ret.X;       // ret.X = p2.Y;
            p2.X = ret.Y;       // ret.Y = p2.X;
            break;
        default:
            SDL_ASSERT(q_3 == quad);
            p2.X = ret.X;       // ret.X = p2.X;
            p2.Y = 0.5 - ret.Y; // ret.Y = 0.5 - p2.Y;
            break;
        }
    }
    if (quad & 1) { // 1, 3
        SDL_ASSERT_1(frange(p2.X, 0, 1));
        SDL_ASSERT_1(frange(p2.Y, 0, 0.25));
        p2.X /= use::scale_13.X;
        p2.Y /= use::scale_13.Y;
    }
    else { // 0, 2
        SDL_ASSERT_1(frange(p2.X, 0, 0.5));
        SDL_ASSERT_1(frange(p2.Y, 0, 0.5));
        p2.X /= use::scale_02.X;
        p2.Y /= use::scale_02.Y;
    }
    //2) re-project 2D to 3D 
    SDL_ASSERT_1(frange(p2.X, 0, use::lx));
    SDL_ASSERT_1(frange(p2.Y, 0, use::ly));
    return use::e1 + multiply(use::px, p2.X) + multiply(use::py, p2.Y);
}

point_2D math::project_globe(spatial_point const & s, hemisphere const h)
{
    SDL_ASSERT(s.is_valid());      
    const quadrant quad = longitude_quadrant(s.longitude);
    const double meridian = longitude_meridian(s.longitude, quad);
    SDL_ASSERT((meridian >= 0) && (meridian <= 90));    
    const point_3D p3 = line_plane_intersect((hemisphere::north == h) ? s.latitude : -s.latitude, meridian);
    return scale_plane_intersect(p3, quad, h);
}

inline point_2D math::project_globe(spatial_point const & s) {
    return math::project_globe(s, latitude_hemisphere(s));
}

inline point_2D math::project_globe(Latitude const lat, Longitude const lon) {
    return project_globe(SP::init(lat, lon));
}

spatial_point math::reverse_project_globe(point_2D const & p2)
{
    const quadrant quad = point_quadrant(p2);
    const hemisphere is_north = point_hemisphere(p2);
    const point_3D p3 = reverse_scale_plane_intersect(p2, quad, is_north);
    spatial_point ret = reverse_line_plane_intersect(p3);
    if (is_north != hemisphere::north) {
        ret.latitude *= -1;
    }
    if (fequal(a_abs(ret.latitude), 90)) {
        ret.longitude = 0;
    }
    else {
        ret.longitude = reverse_longitude_meridian(ret.longitude, quad);
    }
    SDL_ASSERT(ret.is_valid());
    return ret;
}

namespace globe_to_cell_ {

inline int min_max_1(const double p, const int _max) {
    return a_max<int>(a_min<int>(static_cast<int>(p), _max), 0);
};
#if 1
inline point_XY<int> min_max(const point_2D & p, const int _max) {
    return{
        a_max<int>(a_min<int>(static_cast<int>(p.X), _max), 0),
        a_max<int>(a_min<int>(static_cast<int>(p.Y), _max), 0)
    };
};
#else
inline point_XY<int> min_max(const point_2D & p, const int _max) {
    return {
        min_max_1(p.X, _max),
        min_max_1(p.Y, _max)
    };
};
#endif
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

spatial_cell math::make_cell(XY const & p_0, spatial_grid const grid)
{
    using namespace globe_to_cell_;
    const int s_0 = grid.s_0();
    const int s_1 = grid.s_1();
    const int s_2 = grid.s_2();
    SDL_ASSERT(p_0.X >= 0);
    SDL_ASSERT(p_0.Y >= 0);
    SDL_ASSERT(p_0.X < grid.s_3());
    SDL_ASSERT(p_0.Y < grid.s_3());
    const XY h_0 = div_XY(p_0, s_2);
    const XY p_1 = mod_XY(p_0, h_0, s_2);
    const XY h_1 = div_XY(p_1, s_1);
    const XY p_2 = mod_XY(p_1, h_1, s_1);
    const XY h_2 = div_XY(p_2, s_0);
    const XY h_3 = mod_XY(p_2, h_2, s_0);
    SDL_ASSERT((h_0.X >= 0) && (h_0.X < grid[0]));
    SDL_ASSERT((h_0.Y >= 0) && (h_0.Y < grid[0]));
    SDL_ASSERT((h_1.X >= 0) && (h_1.X < grid[1]));
    SDL_ASSERT((h_1.Y >= 0) && (h_1.Y < grid[1]));
    SDL_ASSERT((h_2.X >= 0) && (h_2.X < grid[2]));
    SDL_ASSERT((h_2.Y >= 0) && (h_2.Y < grid[2]));
    SDL_ASSERT((h_3.X >= 0) && (h_3.X < grid[3]));
    SDL_ASSERT((h_3.Y >= 0) && (h_3.Y < grid[3]));
    spatial_cell cell; // uninitialized
    cell[0] = hilbert::xy2d<spatial_cell::id_type>(grid[0], h_0); // hilbert curve distance 
    cell[1] = hilbert::xy2d<spatial_cell::id_type>(grid[1], h_1);
    cell[2] = hilbert::xy2d<spatial_cell::id_type>(grid[2], h_2);
    cell[3] = hilbert::xy2d<spatial_cell::id_type>(grid[3], h_3);
    cell.data.depth = 4;
    return cell;
}

spatial_cell math::globe_to_cell(const point_2D & globe, spatial_grid const grid)
{
    using namespace globe_to_cell_;

    const int g_0 = grid[0];
    const int g_1 = grid[1];
    const int g_2 = grid[2];
    const int g_3 = grid[3];

    SDL_ASSERT_1(frange(globe.X, 0, 1));
    SDL_ASSERT_1(frange(globe.Y, 0, 1));

    const point_XY<int> h_0 = min_max(scale(g_0, globe), g_0 - 1);
    const point_2D fraction_0 = fraction(globe, h_0, g_0);

    SDL_ASSERT_1(frange(fraction_0.X, 0, 1));
    SDL_ASSERT_1(frange(fraction_0.Y, 0, 1));

    const point_XY<int> h_1 = min_max(scale(g_1, fraction_0), g_1 - 1);    
    const point_2D fraction_1 = fraction(fraction_0, h_1, g_1);

    SDL_ASSERT_1(frange(fraction_1.X, 0, 1));
    SDL_ASSERT_1(frange(fraction_1.Y, 0, 1));

    const point_XY<int> h_2 = min_max(scale(g_2, fraction_1), g_2 - 1);    
    const point_2D fraction_2 = fraction(fraction_1, h_2, g_2);

    SDL_ASSERT_1(frange(fraction_2.X, 0, 1));
    SDL_ASSERT_1(frange(fraction_2.Y, 0, 1));

    const point_XY<int> h_3 = min_max(scale(g_3, fraction_2), g_3 - 1);
    spatial_cell cell; // uninitialized
    cell[0] = hilbert::xy2d<spatial_cell::id_type>(g_0, h_0); // hilbert curve distance 
    cell[1] = hilbert::xy2d<spatial_cell::id_type>(g_1, h_1);
    cell[2] = hilbert::xy2d<spatial_cell::id_type>(g_2, h_2);
    cell[3] = hilbert::xy2d<spatial_cell::id_type>(g_3, h_3);
    cell.data.depth = 4;
    return cell;
}

/*
https://en.wikipedia.org/wiki/Haversine_formula
http://www.movable-type.co.uk/scripts/gis-faq-5.1.html
Haversine Formula (from R.W. Sinnott, "Virtues of the Haversine", Sky and Telescope, vol. 68, no. 2, 1984, p. 159):
dlon = lon2 - lon1
dlat = lat2 - lat1
a = sin^2(dlat/2) + cos(lat1) * cos(lat2) * sin^2(dlon/2)
c = 2 * arcsin(min(1,sqrt(a)))
d = R * c 
The great circle distance d will be in the same units as R */
Meters math::haversine(spatial_point const & _1, spatial_point const & _2, const Meters R)
{
    const double dlon = limits::DEG_TO_RAD * (_2.longitude - _1.longitude);
    const double dlat = limits::DEG_TO_RAD * (_2.latitude - _1.latitude);
    const double sin_lat = sin(dlat / 2);
    const double sin_lon = sin(dlon / 2);
    const double a = sin_lat * sin_lat + 
        cos(limits::DEG_TO_RAD * _1.latitude) * 
        cos(limits::DEG_TO_RAD * _2.latitude) * sin_lon * sin_lon;
    const double c = 2 * asin(a_min(1.0, sqrt(a)));
    return c * R.value();
}

inline Meters math::haversine(spatial_point const & p1, spatial_point const & p2)
{
    return haversine(p1, p2, earth_radius(p1.latitude, p2.latitude));
}

/*
http://www.movable-type.co.uk/scripts/latlong.html
http://williams.best.vwh.net/avform.htm#LL
Destination point given distance and bearing from start point
Given a start point, initial bearing, and distance, 
this will calculate the destination point and final bearing travelling along a (shortest distance) great circle arc.
var lat2 = Math.asin( Math.sin(lat1)*Math.cos(d/R) + Math.cos(lat1)*Math.sin(d/R)*Math.cos(brng) );
var lon2 = lon1 + Math.atan2(Math.sin(brng)*Math.sin(d/R)*Math.cos(lat1), Math.cos(d/R)-Math.sin(lat1)*Math.sin(lat2)); */
spatial_point math::destination(spatial_point const & p, Meters const distance, Degree const bearing)
{
    SDL_ASSERT(frange(bearing.value(), 0, 360)); // clockwize direction to north [0..360]
    if (distance.value() <= 0) {
        return p;
    }
    const double radius = earth_radius(p.latitude); // in meters    
    const double dist = distance.value() / radius; // angular distance in radians
    const double brng = bearing.value() * limits::DEG_TO_RAD;
    const double lat1 = p.latitude * limits::DEG_TO_RAD;
    const double lon1 = p.longitude * limits::DEG_TO_RAD;
    const double lat2 = std::asin(std::sin(lat1) * std::cos(dist) + std::cos(lat1) * std::sin(dist) * std::cos(brng));
    const double x = std::cos(dist) - std::sin(lat1) * std::sin(lat2);
    const double y = std::sin(brng) * std::sin(dist) * std::cos(lat1);
    const double lon2 = lon1 + fatan2(y, x);
    spatial_point dest;
    dest.latitude = norm_latitude(lat2 * limits::RAD_TO_DEG);
    dest.longitude = latitude_pole(p.latitude) ?
        norm_longitude(bearing.value()) : // pole is special/rare case
        norm_longitude(lon2 * limits::RAD_TO_DEG);
    SDL_ASSERT(dest.is_valid());
    return dest;
}

point_XY<int> math::quadrant_grid(quadrant const quad, int const grid) {
    SDL_ASSERT(quad <= 3);
    point_XY<int> size;
    if (quad & 1) { // 1, 3
        size.X = grid;
        size.Y = grid / 4;
    }
    else {
        size.X = grid / 2;
        size.Y = grid / 2;
    }
    return size;
}

inline point_XY<int> math::multiply_grid(point_XY<int> const & p, int const grid) {
    return { p.X * grid, p.Y * grid };
}

math::quadrant math::point_quadrant(point_2D const & p) {
    const bool is_north = (p.Y >= 0.5);
    point_2D const pole{ 0.5, is_north ? 0.75 : 0.25 };
    point_2D const vec { p.X - pole.X, p.Y - pole.Y };
    double arg = polar(vec).arg; // in radians
    if (!is_north) {
        arg *= -1.0;
    }
    if (arg >= 0) {
        if (arg <= limits::ATAN_1_2)
            return q_0; 
        if (arg <= limits::PI - limits::ATAN_1_2)
            return q_1; 
    }
    else {
        if (arg >= -limits::ATAN_1_2)
            return q_0; 
        if (arg >= limits::ATAN_1_2 - limits::PI)
            return q_3;
    }
    return q_2;
}

bool math::destination_rect(spatial_rect & rc, spatial_point const & where, Meters const radius) {
    const double degree = limits::RAD_TO_DEG * radius.value() / math::earth_radius(where.latitude);
    rc.min_lat = add_latitude(where.latitude, -degree);
    rc.max_lat = add_latitude(where.latitude, degree);
    rc.min_lon = destination(where, radius, Degree(270)).longitude; // left direction
    rc.max_lon = destination(where, radius, Degree(90)).longitude; // right direction
    if ((rc.max_lat != (where.latitude + degree)) ||
        (rc.min_lat != (where.latitude - degree))) {
        return false; // returns false if wrap over pole
    }
    SDL_ASSERT(rc);
    SDL_ASSERT(where.latitude > rc.min_lat);
    SDL_ASSERT(where.latitude < rc.max_lat);
    return true;
}

bool math::rect_cross_quadrant(spatial_rect const & rc) {
    for (size_t i = 0; i < 4; ++i) {
        if (math::cross_longitude(sorted_quadrant[i], rc.min_lon, rc.max_lon)) {
            return true;
        }
    }
    return false;
}

bool math::cross_longitude(double mid, double left, double right) {
    SDL_ASSERT(SP::valid_longitude(mid));
    SDL_ASSERT(SP::valid_longitude(left));
    SDL_ASSERT(SP::valid_longitude(right));
    if (mid < 0) mid += 360;
    if (left < 0) left += 360;
    if (right < 0) right += 360;
    if (left <= right) {
        return (left < mid) && (mid < right);
    }
    return (left < mid) || (mid < right);
}

double math::longitude_distance(double left, double right) {
    SDL_ASSERT(SP::valid_longitude(left));
    SDL_ASSERT(SP::valid_longitude(right));
    if (left < 0) left += 360;
    if (right < 0) right += 360;
    if (left <= right) {
        return right - left;
    }
    return 360 - (left - right);
}

inline double math::longitude_distance(double const lon1, double const lon2, bool const change_direction) {
    const double d = longitude_distance(lon1, lon2);
    return change_direction ? -d : d;
}

void math::polygon_latitude(vector_point_2D & dest,
                            double const lat, 
                            double const _lon1,
                            double const _lon2,
                            hemisphere const h,
                            bool const change_direction) {
    SDL_ASSERT(_lon1 != _lon2);
    double const lon1 = change_direction ? _lon2 : _lon1;
    double const lon2 = change_direction ? _lon1 : _lon2;
    double const ld = longitude_distance(_lon1, _lon2, change_direction);
    SDL_ASSERT_1(change_direction ? frange(ld, -90, 0) : frange(ld, 0, 90));
    spatial_point const p1 = SP::init(Latitude(lat), Longitude(lon1));
    spatial_point const p2 = SP::init(Latitude(lat), Longitude(lon2));
    Meters const distance = math::haversine(p1, p2);
    enum { min_num = 3 }; // must be odd    
    size_t const num = min_num + static_cast<size_t>(distance.value() / 100000) * 2; //FIXME: experimental, must be odd
    double const step = ld / (num + 1);
    dest.reserve(dest.size() + num + 2);
    dest.push_back(project_globe(p1, h));
    SP mid;
    mid.latitude = lat;
    for (size_t i = 1; i <= num; ++i) {
        mid.longitude = add_longitude(lon1, step * i);
        dest.push_back(project_globe(mid, h));
    }
    dest.push_back(project_globe(p2, h));
}

#if SDL_DEBUG
void debug_trace(vector_point_2D const & v, int N = 1) {
    static int count = 0;
    if (count++ < N) {
        std::cout << "\ntrace(" << count << "):\n";
        size_t i = 0;
        for (auto & p : v) {
            std::cout << (i++)
                << "," << p.X
                << "," << p.Y
                << "\n";
        }
    }
}
#endif

void math::polygon_contour(vector_point_2D & dest, spatial_rect const & rc, hemisphere const h) {
    polygon_latitude(dest, rc.min_lat, rc.min_lon, rc.max_lon, h, false);
    polygon_latitude(dest, rc.max_lat, rc.min_lon, rc.max_lon, h, true);
    SDL_ASSERT(dest.size() >= 4);
}

inline bool point_frange(point_2D const & test,
                         double const x1, double const x2, 
                         double const y1, double const y2) {
    SDL_ASSERT(x1 <= x2);
    SDL_ASSERT(y1 <= y2);
    return frange(test.X, x1, x2) && frange(test.Y, y1, y2);
}

void math::vertical_fill(vector_cell & result,
                         point_2D const * const pp, 
                         point_2D const * const pp_end, 
                         spatial_grid const grid,
                         vector_point_2D const * const not_used)
{
    SDL_ASSERT(pp + 4 <= pp_end);
    pair_size_t const index = find_range(pp, pp_end, [](point_2D const & p1, point_2D const & p2) {
        return p1.X < p2.X;
    });
    SDL_ASSERT(index.first != index.second);
    SDL_ASSERT(pp[index.first].X <= pp[index.second].X);
    if (index.first == index.second) {
        SDL_ASSERT(index.first == 0);
        result.push_back(globe_to_cell(pp[0], grid));
        return;
    }
    const int max_id = grid.s_3();
    double const grid_step = grid.f_3();
    size_t const last = pp_end - pp - 1;
    size_t i = index.first;
    size_t j = index.first;
    while ((i != index.second) && (j != index.second)) {
        size_t const inext = (i == last) ? 0 : (i + 1);
        size_t const jnext = j ? (j - 1) : last;
        point_2D const & p_i = pp[i];
        point_2D const & p_j = pp[j];
        point_2D const & p_inext = pp[inext];
        point_2D const & p_jnext = pp[jnext];
        double const x1 = a_max(p_i.X, p_j.X);
        double const x2 = a_min(p_inext.X, p_jnext.X);
        SDL_ASSERT(pp[index.first].X <= x1);
        SDL_ASSERT(pp[index.second].X >= x2);
        SDL_ASSERT(x1 <= x2);
        SDL_ASSERT(p_i.X <= p_inext.X);
        SDL_ASSERT(p_j.X <= p_jnext.X);
        if (x1 < x2) {
            SDL_ASSERT((p_inext.X > p_i.X) && (p_jnext.X > p_j.X));
            XY cell_pos;
            double y1, y2;
            double const dy1 = (p_inext.Y - p_i.Y) / (p_inext.X - p_i.X);
            double const dy2 = (p_jnext.Y - p_j.Y) / (p_jnext.X - p_j.X);
            for (double x = x1; x <= x2; x += grid_step) {
                y1 = p_i.Y + (x - p_i.X) * dy1;
                y2 = p_j.Y + (x - p_j.X) * dy2;
                if (y2 < y1) {
                    std::swap(y1, y2);
                }
                cell_pos.X = globe_to_cell_::min_max_1(max_id * x, max_id - 1);
                for (double y = y1; y <= y2; y += grid_step) {
                    cell_pos.Y = globe_to_cell_::min_max_1(max_id * y, max_id - 1);
                    result.push_back(math::make_cell(cell_pos, grid));
#if SDL_DEBUG > 1
                    SDL_ASSERT_1(point_frange(
                        transform::cell_point(result.back(), grid),
                        x - grid_step, x,
                        y - grid_step, y));
#endif
                }
            }
        }
        else { // x1 == x2
            SDL_ASSERT(x1 == x2);
            const double y1 = a_min(a_min(p_i.Y, p_inext.Y), a_min(p_j.Y, p_jnext.Y));
            const double y2 = a_max(a_max(p_i.Y, p_inext.Y), a_max(p_j.Y, p_jnext.Y));
            SDL_ASSERT(y1 < y2);
            XY cell_pos;
            for (double y = y1; y <= y2; y += grid_step) {
                cell_pos.X = globe_to_cell_::min_max_1(max_id * x1, max_id - 1);
                cell_pos.Y = globe_to_cell_::min_max_1(max_id * y, max_id - 1);
                result.push_back(math::make_cell(cell_pos, grid));
            }
        }
        if (x2 == p_inext.X) {
            i = inext;
        }
        if (x2 == p_jnext.X) {
            j = jnext;
        }
        SDL_ASSERT((inext == i) || (jnext == j));
    }
    SDL_ASSERT(!result.empty());
}

void math::horizontal_fill(vector_cell & result,
                           point_2D const * const pp, 
                           point_2D const * const pp_end, 
                           spatial_grid const grid,
                           vector_point_2D const * const not_used)
{
    SDL_ASSERT(pp + 4 <= pp_end);
    pair_size_t const index = find_range(pp, pp_end, [](point_2D const & p1, point_2D const & p2) {
        return p1.Y < p2.Y;
    });
    SDL_ASSERT(index.first != index.second);
    SDL_ASSERT(pp[index.first].Y <= pp[index.second].Y);
    if (index.first == index.second) {
        SDL_ASSERT(index.first == 0);
        result.push_back(globe_to_cell(pp[0], grid));
        return;
    }
    const int max_id = grid.s_3();
    double const grid_step = grid.f_3();
    size_t const last = pp_end - pp - 1;
    size_t i = index.first;
    size_t j = index.first;
    while ((i != index.second) && (j != index.second)) {
        size_t const inext = (i == last) ? 0 : (i + 1);
        size_t const jnext = j ? (j - 1) : last;
        point_2D const & p_i = pp[i];
        point_2D const & p_j = pp[j];
        point_2D const & p_inext = pp[inext];
        point_2D const & p_jnext = pp[jnext];
        double const y1 = a_max(p_i.Y, p_j.Y);
        double const y2 = a_min(p_inext.Y, p_jnext.Y);
        SDL_ASSERT(pp[index.first].Y <= y1);
        SDL_ASSERT(pp[index.second].Y >= y2);
        SDL_ASSERT(y1 <= y2);
        SDL_ASSERT(p_i.Y <= p_inext.Y);
        SDL_ASSERT(p_j.Y <= p_jnext.Y);
        if (y1 < y2) {
            SDL_ASSERT((p_inext.Y > p_i.Y) && (p_jnext.Y > p_j.Y));
            XY cell_pos;
            double x1, x2;
            double const dx1 = (p_inext.X - p_i.X) / (p_inext.Y - p_i.Y);
            double const dx2 = (p_jnext.X - p_j.X) / (p_jnext.Y - p_j.Y);
            for (double y = y1; y <= y2; y += grid_step) {
                x1 = p_i.X + (y - p_i.Y) * dx1;
                x2 = p_j.X + (y - p_j.Y) * dx2;
                if (x2 < x1) {
                    std::swap(x1, x2);
                }
                cell_pos.Y = globe_to_cell_::min_max_1(max_id * y, max_id - 1);
                for (double x = x1; x <= x2; x += grid_step) {
                    cell_pos.X = globe_to_cell_::min_max_1(max_id * x, max_id - 1);
                    result.push_back(math::make_cell(cell_pos, grid));
#if SDL_DEBUG > 1
                    SDL_ASSERT_1(point_frange(
                        transform::cell_point(result.back(), grid),
                        x - grid_step, x,
                        y - grid_step, y));
#endif
                }
            }
        }
        else { // y1 == y2
            SDL_ASSERT(y1 == y2);
            const double x1 = a_min(a_min(p_i.X, p_inext.X), a_min(p_j.X, p_jnext.X));
            const double x2 = a_max(a_max(p_i.X, p_inext.X), a_max(p_j.X, p_jnext.X));
            SDL_ASSERT(x1 < x2);
            XY cell_pos;
            for (double x = x1; x <= x2; x += grid_step) {
                cell_pos.X = globe_to_cell_::min_max_1(max_id * x, max_id - 1);
                cell_pos.Y = globe_to_cell_::min_max_1(max_id * y1, max_id - 1);
                result.push_back(math::make_cell(cell_pos, grid));
            }
        }
        if (y2 == p_inext.Y) {
            i = inext;
        }
        if (y2 == p_jnext.Y) {
            j = jnext;
        }
        SDL_ASSERT((inext == i) || (jnext == j));
    }
    SDL_ASSERT(!result.empty());
}

void math::select_sector(vector_cell & result, spatial_rect const & rc, spatial_grid const grid)
{
    SDL_ASSERT(rc && !rc.cross_equator() && !rect_cross_quadrant(rc));
    SDL_ASSERT(fless_eq(longitude_distance(rc.min_lon, rc.max_lon), 90));
    const hemisphere h = latitude_hemisphere((rc.min_lat + rc.max_lat) / 2);
    const quadrant q = longitude_quadrant(rc.min_lon);
    SDL_ASSERT(q <= longitude_quadrant(rc.max_lon));
    vector_point_2D cont;
    polygon_contour(cont, rc, h);
    if (q & 1) { // 1, 3
        vertical_fill(result, cont.data(), cont.data() + cont.size(), grid);
    }
    else { // 0, 2
        horizontal_fill(result, cont.data(), cont.data() + cont.size(), grid); 
    }
}

vector_cell math::select_hemisphere(spatial_rect const & rc, spatial_grid const grid)
{
    SDL_ASSERT(rc && !rc.cross_equator());
    vector_cell result;
    result.reserve(64);
    spatial_rect sector = rc;
    for (size_t i = 0; i < quadrant_size; ++i) {
        double const d = sorted_quadrant[i];
        SDL_ASSERT((0 == i) || (sorted_quadrant[i - 1] < d));
        if (cross_longitude(d, sector.min_lon, sector.max_lon)) {
            SDL_ASSERT(d != sector.min_lon);
            SDL_ASSERT(d != sector.max_lon);
            sector.max_lon = d;
            select_sector(result, sector, grid);
            sector.min_lon = d;
            sector.max_lon = rc.max_lon;
        }
    }
    SDL_ASSERT(sector && (sector.max_lon == rc.max_lon));
    select_sector(result, sector, grid);
    return result;
}

math::sector_indexes
math::polygon_range(vector_point_2D & result, spatial_point const & where, Meters const radius, sector_t const & where_sec)
{
    SDL_ASSERT(radius.value() > 0);
    SDL_ASSERT(where_sec == spatial_sector(where));
    SDL_ASSERT(result.empty());

    enum { min_num = 32 };
    const double degree = limits::RAD_TO_DEG * radius.value() / earth_radius(where);
    const size_t num = roundup(degree * 32, min_num); //FIXME: experimental
    SDL_ASSERT(num && !(num % min_num));
    const double bx = 360.0 / num;
    SDL_ASSERT(frange(bx, 1.0, 360.0 / min_num));

    sector_indexes cross_index;
    result.reserve(result.size() + num);
    spatial_point sp = destination(where, radius, Degree(0)); // bearing = 0
    sector_t sec1 = spatial_sector(sp), sec2;
    if (sec1 != where_sec) {
        cross_index.emplace_back(sec1, result.size()); // result.size() = 0
    }
    result.push_back(project_globe(sp));
    for (double bearing = bx; bearing < 360; bearing += bx) {
        sp = destination(where, radius, Degree(bearing));
        point_2D const next = project_globe(sp);
        if ((sec2 = spatial_sector(sp)) != sec1) {
            if (sec1.h != sec2.h) { // find intersection with equator
                spatial_point half_back = destination(where, radius, Degree(bearing - bx * 0.5));
                half_back.latitude = 0;
#if SDL_DEBUG
                {
                    auto error_meters = a_abs(haversine(where, half_back).value() - radius.value());
                    SDL_ASSERT(error_meters < 100); // error must be small
                }
#endif
                point_2D const mid = math::project_globe(half_back, sec1.h);
                SDL_ASSERT(length(result.back() - mid) < 0.1); // error must be small
                cross_index.emplace_back(sec2, result.size());
                result.push_back(mid);
            }
            else { // find intersection with quadrant
                SDL_ASSERT(sec1.q != sec2.q);
                point_2D const & back = result.back();
                point_2D const mid = { 
                    (back.X + next.X) / 2,
                    (back.Y + next.Y) / 2 
                };
                SDL_ASSERT(length(back - next) < 0.1); // error must be small
                cross_index.emplace_back(sec2, result.size());
                result.push_back(mid);
            }
            sec1 = sec2;
        }
        result.push_back(next);
    }
    return cross_index;
}

void math::select_range_sector(vector_cell & result, 
                        spatial_point const & where, Meters const radius, spatial_grid const grid,
                        point_2D const * const pp, 
                        point_2D const * const pp_end,
                        vector_point_2D const * const not_used)
{
    SDL_ASSERT(pp < pp_end);
    const size_t size = pp_end - pp;
}

namespace select_range_ {

template<class iterator>
iterator cross_hemisphere(iterator first, iterator const last, math::hemisphere const h) {
    for (; first != last; ++first) {
        if (first->first.h != h)
            break;
    }
    return first;
}

} // select_range_

vector_cell math::select_range(spatial_point const & where, Meters const radius, spatial_grid const grid)
{
    using namespace select_range_;
    vector_point_2D cont;
    sector_t const where_sec = spatial_sector(where);
    sector_indexes const cross = polygon_range(cont, where, radius, where_sec);
    SDL_ASSERT(!cont.empty());
    vector_cell result;
    result.reserve(64);
    if (cross.empty()) {
        SDL_ASSERT(!latitude_pole(where.latitude));
        if (where_sec.q & 1) { // 1, 3
            vertical_fill(result, cont.data(), cont.data() + cont.size(), grid
#if SDL_DEBUG
                , &cont
#endif
            );
        }
        else { // 0, 2
            horizontal_fill(result, cont.data(), cont.data() + cont.size(), grid
#if SDL_DEBUG
                , &cont
#endif
            );
        }
        SDL_ASSERT(!result.empty());
        return result;
    }
    else {
        // cross hemisphere ?
        auto it = cross_hemisphere(cross.begin(), cross.end(), where_sec.h);
        if (it != cross.end()) {
            SDL_ASSERT(it->first.h != where_sec.h);
        }
        return result;
    }
}

#if 0
        SDL_ASSERT(cross[0].second);
        size_t index = 0;
        for (auto & s : cross) { // process sectors
            SDL_ASSERT(index < s.second);
            select_range_sector(result, where, radius, grid,
                cont.data() + index,
                cont.data() + s.second
#if SDL_DEBUG
                , &cont
#endif
            );
            index = s.second;
        }
#endif

} // namespace space

using namespace space;

spatial_point transform::spatial(point_2D const & p) {
    return math::reverse_project_globe(p);
}

spatial_cell transform::make_cell(spatial_point const & p, spatial_grid const g) {
    return math::globe_to_cell(math::project_globe(p), g);
}

point_XY<int> transform::d2xy(spatial_cell::id_type const id, grid_size const size) {
    return hilbert::d2xy((int)size, id);
}

point_2D transform::cell_point(spatial_cell const & cell, spatial_grid const grid)
{
    const int g_0 = grid[0];
    const int g_1 = grid[1];
    const int g_2 = grid[2];
    const int g_3 = grid[3];

    const XY p_0 = hilbert::d2xy(g_0, cell[0]);
    const XY p_1 = hilbert::d2xy(g_1, cell[1]);
    const XY p_2 = hilbert::d2xy(g_2, cell[2]);
    const XY p_3 = hilbert::d2xy(g_3, cell[3]);

    const double f_0 = 1.0 / g_0;
    const double f_1 = f_0 / g_1;
    const double f_2 = f_1 / g_2;
    const double f_3 = f_2 / g_3;

    point_2D pos;
    pos.X = p_0.X * f_0;
    pos.Y = p_0.Y * f_0;
    pos.X += p_1.X * f_1;
    pos.Y += p_1.Y * f_1;
    pos.X += p_2.X * f_2;
    pos.Y += p_2.Y * f_2;
    pos.X += p_3.X * f_3;
    pos.Y += p_3.Y * f_3;    
    SDL_ASSERT_1(frange(pos.X, 0, 1));
    SDL_ASSERT_1(frange(pos.Y, 0, 1));
    return pos;
}

vector_cell
transform::cell_rect(spatial_rect const & rc, spatial_grid const grid)
{
    using namespace space;
    if (!rc) {
        SDL_ASSERT(0); // not implemented
        return{};
    }
    if (rc.cross_equator()) {
        SDL_ASSERT((rc.min_lat < 0) && (0 < rc.max_lat));
        spatial_rect r1 = rc;
        spatial_rect r2 = rc;
        r1.min_lat = 0; // [0..max_lat] north
        r2.max_lat = 0; // [min_lat..0] south
        return sort_unique(
            math::select_hemisphere(r1, grid),
            math::select_hemisphere(r2, grid));
    }
    return sort_unique(math::select_hemisphere(rc, grid));
}

#if 0 // approximation
vector_cell
transform::cell_range(spatial_point const & where, Meters const radius, spatial_grid const grid)
{
    if (!fless_eq(radius.value(), 0)) {
        spatial_rect rc;
        if (math::destination_rect(rc, where, radius)) { 
            return cell_rect(rc, grid);
        }
        SDL_WARNING(!"wrap over pole");
    }
    return { make_cell(where, grid) };
}
#else
vector_cell
transform::cell_range(spatial_point const & where, Meters const radius, spatial_grid const grid)
{
    if (!fless_eq(radius.value(), 0)) {
        return sort_unique(math::select_range(where, radius, grid));
    }
    return { make_cell(where, grid) };
}
#endif

} // db
} // sdl

#if SDL_DEBUG
namespace sdl {
    namespace db {
        namespace {
            class unit_test {
            public:
                unit_test()
                {
                    test_hilbert();
                    test_spatial();
                    {
                        A_STATIC_ASSERT_TYPE(point_2D::type, double);
                        A_STATIC_ASSERT_TYPE(point_3D::type, double);                        
                        SDL_ASSERT_1(math::cartesian(Latitude(0), Longitude(0)) == point_3D{1, 0, 0});
                        SDL_ASSERT_1(math::cartesian(Latitude(0), Longitude(90)) == point_3D{0, 1, 0});
                        SDL_ASSERT_1(math::cartesian(Latitude(90), Longitude(0)) == point_3D{0, 0, 1});
                        SDL_ASSERT_1(math::cartesian(Latitude(90), Longitude(90)) == point_3D{0, 0, 1});
                        SDL_ASSERT_1(math::cartesian(Latitude(45), Longitude(45)) == point_3D{0.5, 0.5, 0.70710678118654752440});
                        SDL_ASSERT_1(math::line_plane_intersect(Latitude(0), Longitude(0)) == point_3D{1, 0, 0});
                        SDL_ASSERT_1(math::line_plane_intersect(Latitude(0), Longitude(90)) == point_3D{0, 1, 0});
                        SDL_ASSERT_1(math::line_plane_intersect(Latitude(90), Longitude(0)) == point_3D{0, 0, 1});
                        SDL_ASSERT_1(math::line_plane_intersect(Latitude(90), Longitude(90)) == point_3D{0, 0, 1});
                        SDL_ASSERT_1(fequal(length(math::line_plane_intersect(Latitude(45), Longitude(45))), 0.58578643762690497));
                        SDL_ASSERT_1(math::longitude_quadrant(0) == 0);
                        SDL_ASSERT_1(math::longitude_quadrant(45) == 1);
                        SDL_ASSERT_1(math::longitude_quadrant(90) == 1);
                        SDL_ASSERT_1(math::longitude_quadrant(135) == 2);
                        SDL_ASSERT_1(math::longitude_quadrant(180) == 2);
                        SDL_ASSERT_1(math::longitude_quadrant(-45) == 0);
                        SDL_ASSERT_1(math::longitude_quadrant(-90) == 3);
                        SDL_ASSERT_1(math::longitude_quadrant(-135) == 3);
                        SDL_ASSERT_1(math::longitude_quadrant(-180) == 2);
                        SDL_ASSERT(fequal(limits::ATAN_1_2, std::atan2(1, 2)));
#if !defined(SDL_VISUAL_STUDIO_2013)
                        static_assert(fsign(0) == 0, "");
                        static_assert(fsign(1) == 1, "");
                        static_assert(fsign(-1) == -1, "");
                        static_assert(fzero(0), "");
                        static_assert(fzero(limits::fepsilon), "");
                        static_assert(!fzero(limits::fepsilon * 2), "");
                        static_assert(a_min_max(0.5, 0.0, 1.0) == 0.5, "");
                        static_assert(a_min_max(-1.0, 0.0, 1.0) == 0.0, "");
                        static_assert(a_min_max(2.5, 0.0, 1.0) == 1.0, "");
                        static_assert(reverse_bytes(0x01020304) == 0x04030201, "reverse_bytes");
#endif
                    }
                    if (1)
                    {
                        spatial_cell x{}, y{};
                        SDL_ASSERT(!spatial_cell::less(x, y));
                        SDL_ASSERT(x == y);
                        y.set_depth(1);
                        SDL_ASSERT(x != y);
                        SDL_ASSERT(spatial_cell::less(x, y));
                        SDL_ASSERT(!spatial_cell::less(y, x));
                    }
                    if (0) { // generate static tables
                        std::cout << "\nd2xy:\n";
                        enum { HIGH = spatial_grid::HIGH };
                        int dist[HIGH][HIGH]{};
                        for (int i = 0; i < HIGH; ++i) {
                            for (int j = 0; j < HIGH; ++j) {
                                const int d = i * HIGH + j;
                                point_XY<int> const h = transform::d2xy(d);
                                dist[h.X][h.Y] = d;
                                std::cout << "{" << h.X << "," << h.Y << "},";
                            }
                            std::cout << " // " << i << "\n";
                        }
                        std::cout << "\nxy2d:";
                        for (int x = 0; x < HIGH; ++x) {
                            std::cout << "\n{";
                            for (int y = 0; y < HIGH; ++y) {
                                if (y) std::cout << ",";
                                std::cout << dist[x][y];
                            }
                            std::cout << "}, // " << x;
                        }
                        std::cout << std::endl;
                    }
                    if (1) {
                        SDL_ASSERT(fequal(math::norm_longitude(0), 0));
                        SDL_ASSERT(fequal(math::norm_longitude(180), 180));
                        SDL_ASSERT(fequal(math::norm_longitude(-180), -180));
                        SDL_ASSERT(fequal(math::norm_longitude(-180 - 90), 90));
                        SDL_ASSERT(fequal(math::norm_longitude(180 + 90), -90));
                        SDL_ASSERT(fequal(math::norm_longitude(180 + 90 + 360), -90));
                        SDL_ASSERT(fequal(math::norm_latitude(0), 0));
                        SDL_ASSERT(fequal(math::norm_latitude(-90), -90));
                        SDL_ASSERT(fequal(math::norm_latitude(90), 90));
                        SDL_ASSERT(fequal(math::norm_latitude(90+10), 80));
                        SDL_ASSERT(fequal(math::norm_latitude(90+10+360), 80));
                        SDL_ASSERT(fequal(math::norm_latitude(-90-10), -80));
                        SDL_ASSERT(fequal(math::norm_latitude(-90-10-360), -80));
                        SDL_ASSERT(fequal(math::norm_latitude(-90-10+360), -80));
                    }
                    if (1) {
                        SDL_ASSERT(math::point_quadrant(point_2D{}) == 1);
                        SDL_ASSERT(math::point_quadrant(point_2D{0, 0.25}) == 2);
                        SDL_ASSERT(math::point_quadrant(point_2D{0.5, 0.375}) == 3);
                        SDL_ASSERT(math::point_quadrant(point_2D{0.5, 0.5}) == 3);
                        SDL_ASSERT(math::point_quadrant(point_2D{1.0, 0.25}) == 0);
                        SDL_ASSERT(math::point_quadrant(point_2D{1.0, 0.75}) == 0);
                        SDL_ASSERT(math::point_quadrant(point_2D{1.0, 1.0}) == 0);
                        SDL_ASSERT(math::point_quadrant(point_2D{0.5, 1.0}) == 1);
                        SDL_ASSERT(math::point_quadrant(point_2D{0, 0.75}) == 2);
                    }
                    if (1) {
                        {
                            double const earth_radius = math::earth_radius(Latitude(0)); // depends on EARTH_ELLIPSOUD
                            Meters const d1 = earth_radius * limits::PI / 2;
                            Meters const d2 = d1.value() / 2;
                            SDL_ASSERT(math::destination(SP::init(Latitude(0), Longitude(0)), d1, Degree(0)).equal(Latitude(90), Longitude(0)));
                            SDL_ASSERT(math::destination(SP::init(Latitude(0), Longitude(0)), d1, Degree(360)).equal(Latitude(90), Longitude(0)));
                            SDL_ASSERT(math::destination(SP::init(Latitude(0), Longitude(0)), d2, Degree(0)).equal(Latitude(45), Longitude(0)));
                            SDL_ASSERT(math::destination(SP::init(Latitude(0), Longitude(0)), d2, Degree(90)).equal(Latitude(0), Longitude(45)));
                            SDL_ASSERT(math::destination(SP::init(Latitude(0), Longitude(0)), d2, Degree(180)).equal(Latitude(-45), Longitude(0)));
                            SDL_ASSERT(math::destination(SP::init(Latitude(0), Longitude(0)), d2, Degree(270)).equal(Latitude(0), Longitude(-45)));
                        }
                        {
                            double const earth_radius = math::earth_radius(Latitude(90)); // depends on EARTH_ELLIPSOUD
                            Meters const d1 = earth_radius * limits::PI / 2;
                            Meters const d2 = d1.value() / 2;
                            SDL_ASSERT(math::destination(SP::init(Latitude(90), Longitude(0)), d2, Degree(0)).equal(Latitude(45), Longitude(0)));
                            SDL_ASSERT(math::destination(SP::init(Latitude(-90), Longitude(0)), d2, Degree(0)).equal(Latitude(-45), Longitude(0)));
                        }
                    }
                    if (1) { // 111 km arc of circle => line chord => 1.4 meter error
                        double constexpr R = limits::EARTH_MINOR_RADIUS;    // 6356752.3142449996 meters
                        double constexpr angle = 1.0 * limits::DEG_TO_RAD;  // 0.017453292519943295 radian
                        double constexpr L = angle * R;                     // 110946.25761734448 meters
                        double const H = 2 * R * std::sin(angle / 2);       // 110944.84944925930 meters
                        double const delta = L - H;                         // 1.4081680851813871 meters
                        SDL_ASSERT(fequal(delta, 1.4081680851813871));
                    }
                    if (1) {
                        draw_grid(false);
                        reverse_grid(false);
                    }
                    if (1) {
                        interval_cell test;
                        spatial_cell c1, c2;
                        c1.data.depth = 4;
                        c1[0] = 1;
                        c1[1] = 2;
                        c1[2] = 3;
                        c1[3] = 4;
                        c2.data.depth = 4;
                        c2[0] = 2;
                        c2[1] = 3;
                        c2[2] = 1;
                        c2[3] = 0;
                        SDL_ASSERT(c1 < c2);
                        SDL_ASSERT(interval_cell::compare::less(c1, c2));
                        SDL_ASSERT(test.insert(c1));
                        SDL_ASSERT(test.insert(c2));
                        c2[3] = 2; SDL_ASSERT(test.insert(c2));
                        c2[3] = 1; SDL_ASSERT(test.insert(c2));
                        if (1) {
                            SDL_ASSERT(test.insert(spatial_cell::min()));
                            SDL_ASSERT(!test.insert(spatial_cell::min()));
                            SDL_ASSERT(test.insert(spatial_cell::max()));
                            c1 = spatial_cell::max();
                            c1[3] = 0;
                            SDL_ASSERT(test.insert(c1));
                        }
                    }
                }
            private:
                static void trace_hilbert(const int n) {
                    for (int y = 0; y < n; ++y) {
                        std::cout << y;
                        for (int x = 0; x < n; ++x) {
                            const int d = hilbert::xy2d(n, x, y);
                            std::cout << "," << d;
                        }
                        std::cout << std::endl;
                    }
                }
                static void test_hilbert(const int n) {
                    for (int d = 0; d < (n * n); ++d) {
                        int x = 0, y = 0;
                        hilbert::d2xy(n, d, x, y);
                        SDL_ASSERT(d == hilbert::xy2d(n, x, y));
                        //SDL_TRACE("d2xy: n = ", n, " d = ", d, " x = ", x, " y = ", y);
#if is_static_hilbert
                        if (n == spatial_grid::HIGH) {
                            SDL_ASSERT(hilbert::static_d2xy[d].X == x);
                            SDL_ASSERT(hilbert::static_d2xy[d].Y == y);
                            SDL_ASSERT(hilbert::static_xy2d[x][y] == d);
                        }
#endif
                    }
                }
                static void test_hilbert() {
                    spatial_grid::grid_size const sz = spatial_grid::HIGH;
                    for (int i = 0; (1 << i) <= sz; ++i) {
                        test_hilbert(1 << i);
                    }
                }
                static void trace_cell(const spatial_cell & cell) {
                    //SDL_TRACE(to_string::type(cell));
                }
                static void test_spatial(const spatial_grid & grid) {
                    if (1) {
                        spatial_point p1{}, p2{};
                        for (int i = 0; i <= 4; ++i) {
                        for (int j = 0; j <= 2; ++j) {
                            p1.longitude = 45 * i; 
                            p2.longitude = -45 * i;
                            p1.latitude = 45 * j;
                            p2.latitude = -45 * j;
                            math::project_globe(p1);
                            math::project_globe(p2);
#if high_grid_optimization
                            transform::make_cell(p1, spatial_grid());
#else
                            transform::make_cell(p1, spatial_grid(spatial_grid::LOW));
                            transform::make_cell(p1, spatial_grid(spatial_grid::MEDIUM));
                            transform::make_cell(p1, spatial_grid(spatial_grid::HIGH));
#endif
                        }}
                    }
                    if (1) {
                        static const spatial_point test[] = { // latitude, longitude
                            { 48.7139, 44.4984 },   // cell_id = 156-163-67-177-4
                            { 55.7975, 49.2194 },   // cell_id = 157-178-149-55-4
                            { 47.2629, 39.7111 },   // cell_id = 163-78-72-221-4
                            { 47.261, 39.7068 },    // cell_id = 163-78-72-223-4
                            { 55.7831, 37.3567 },   // cell_id = 156-38-25-118-4
                            { 0, -86 },             // cell_id = 128-234-255-15-4
                            { 45, -135 },           // cell_id = 70-170-170-170-4 | 73-255-255-255-4 | 118-0-0-0-4 | 121-85-85-85-4 
                            { 45, 135 },            // cell_id = 91-255-255-255-4 | 92-170-170-170-4 | 99-85-85-85-4 | 100-0-0-0-4
                            { 45, 0 },              // cell_id = 160-236-255-239-4 | 181-153-170-154-4
                            { 45, -45 },            // cell_id = 134-170-170-170-4 | 137-255-255-255-4 | 182-0-0-0-4 | 185-85-85-85-4
                            { 0, 0 },               // cell_id = 175-255-255-255-4 | 175-255-255-255-4
                            { 0, 135 },
                            { 0, 90 },
                            { 90, 0 },
                            { -90, 0 },
                            { 0, -45 },
                            { 45, 45 },
                            { 0, 180 },
                            { 0, -180 },
                            { 0, 131 },
                            { 0, 134 },
                            { 0, 144 },
                            { 0, 145 },
                            { 0, 166 },             // cell_id = 5-0-0-79-4 | 80-85-85-58-4
                        };
                        for (size_t i = 0; i < A_ARRAY_SIZE(test); ++i) {
                            //std::cout << i << ": " << to_string::type(test[i]) << " => ";
                            trace_cell(transform::make_cell(test[i], grid));
                        }
                    }
                    if (1) {
                        spatial_point p1 {};
                        spatial_point p2 {};
                        SDL_ASSERT(fequal(math::haversine(p1, p2).value(), 0));
                        {
                            p2.latitude = 90.0 / 16;
                            const double h1 = math::haversine(p1, p2, limits::EARTH_RADIUS).value();
                            const double h2 = p2.latitude * limits::DEG_TO_RAD * limits::EARTH_RADIUS;
                            SDL_ASSERT(fequal(h1, h2));
                        }
                        {
                            p2.latitude = 90.0;
                            const double h1 = math::haversine(p1, p2, limits::EARTH_RADIUS).value();
                            const double h2 = p2.latitude * limits::DEG_TO_RAD * limits::EARTH_RADIUS;
                            SDL_ASSERT(fless(a_abs(h1 - h2), 1e-08));
                        }
                        if (math::EARTH_ELLIPSOUD) {
                            SDL_ASSERT(fequal(math::earth_radius(0), limits::EARTH_MAJOR_RADIUS));
                            SDL_ASSERT(fequal(math::earth_radius(90), limits::EARTH_MINOR_RADIUS));
                        }
                        else {
                            SDL_ASSERT(fequal(math::earth_radius(0), limits::EARTH_RADIUS));
                            SDL_ASSERT(fequal(math::earth_radius(90), limits::EARTH_RADIUS));
                        }
                    }
                }
                static void test_spatial() {
                    test_spatial(spatial_grid());
                }
                static void draw_grid(bool const print) {
                    if (1) {
                        if (print) {
                            std::cout << "\ndraw_grid:\n";
                        }
                        const double sx = 16 * 4;
                        const double sy = 16 * 2;
                        const double dy = (SP::max_latitude - SP::min_latitude) / sy;
                        const double dx = (SP::max_longitude - SP::min_longitude) / sx;
                        size_t i = 0;
                        for (double y = SP::min_latitude; y <= SP::max_latitude; y += dy) {
                        for (double x = SP::min_longitude; x <= SP::max_longitude; x += dx) {
                            point_2D const p2 = math::project_globe(Latitude(y), Longitude(x));
                            if (print) {
                                std::cout << (i++) 
                                    << "," << p2.X
                                    << "," << p2.Y
                                    << "," << x
                                    << "," << y
                                    << "\n";
                            }
                            SP const g = math::reverse_project_globe(p2);
                            SDL_ASSERT(g.match(SP::init(Latitude(y), Longitude(x))));
                        }}
                    }
                    if (0) {
                        draw_circle(SP::init(Latitude(45), Longitude(0)), Meters(1000 * 1000));
                        draw_circle(SP::init(Latitude(0), Longitude(0)), Meters(1000 * 1000));
                        draw_circle(SP::init(Latitude(60), Longitude(45)), Meters(1000 * 1000));
                        draw_circle(SP::init(Latitude(85), Longitude(30)), Meters(1000 * 1000));
                        draw_circle(SP::init(Latitude(-60), Longitude(30)), Meters(1000 * 500));
                    }
                }
                static void draw_circle(SP const center, Meters const distance) {
                    //std::cout << "\ndraw_circle:\n";
                    const double bx = 1;
                    size_t i = 0;
                    for (double bearing = 0; bearing < 360; bearing += bx) {
                        SP const sp = math::destination(center, distance, Degree(bearing));
                        point_2D const p = math::project_globe(sp);
                        std::cout << (i++) 
                            << "," << p.X
                            << "," << p.Y
                            << "," << sp.longitude
                            << "," << sp.latitude
                            << "\n";
                    }
                }
                static void reverse_grid(bool const print) {
                    if (print) {
                        std::cout << "\nreverse_grid:\n";
                    }
                    size_t i = 0;
                    const double d = spatial_grid().f_0() / 2.0;
                    for (double x = 0; x <= 1.0; x += d) {
                        for (double y = 0; y <= 1.0; y += d) {
                            point_2D const p1{ x, y };
                            SP const g1 = math::reverse_project_globe(p1);
                            if (print) {
                                std::cout << (i++) 
                                    << "," << p1.X
                                    << "," << p1.Y
                                    << "," << g1.longitude
                                    << "," << g1.latitude
                                    << "\n";
                            }
                            if (!fzero(g1.latitude)) {
                                point_2D const p2 = math::project_globe(g1);
                                SDL_ASSERT(p2 == p1);
                            }
                        }
                    }
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG

#if 0
void fill_poly_v2i_n(
        const int xmin, const int ymin, const int xmax, const int ymax,
        const int verts[][2], const int nr,
        void (*callback)(int, int, void *), void *userData)
{
	/* originally by Darel Rex Finley, 2007 */

	int  nodes, pixel_y, i, j, swap;
	int *node_x = MEM_mallocN(sizeof(*node_x) * (size_t)(nr + 1), __func__);

	/* Loop through the rows of the image. */
	for (pixel_y = ymin; pixel_y < ymax; pixel_y++) {

		/* Build a list of nodes. */
		nodes = 0; j = nr - 1;
		for (i = 0; i < nr; i++) {
			if ((verts[i][1] < pixel_y && verts[j][1] >= pixel_y) ||
			    (verts[j][1] < pixel_y && verts[i][1] >= pixel_y))
			{
				node_x[nodes++] = (int)(verts[i][0] +
				                        ((double)(pixel_y - verts[i][1]) / (verts[j][1] - verts[i][1])) *
				                        (verts[j][0] - verts[i][0]));
			}
			j = i;
		}

		/* Sort the nodes, via a simple "Bubble" sort. */
		i = 0;
		while (i < nodes - 1) {
			if (node_x[i] > node_x[i + 1]) {
				SWAP_TVAL(swap, node_x[i], node_x[i + 1]);
				if (i) i--;
			}
			else {
				i++;
			}
		}

		/* Fill the pixels between node pairs. */
		for (i = 0; i < nodes; i += 2) {
			if (node_x[i] >= xmax) break;
			if (node_x[i + 1] >  xmin) {
				if (node_x[i    ] < xmin) node_x[i    ] = xmin;
				if (node_x[i + 1] > xmax) node_x[i + 1] = xmax;
				for (j = node_x[i]; j < node_x[i + 1]; j++) {
					callback(j - xmin, pixel_y - ymin, userData);
				}
			}
		}
	}
	MEM_freeN(node_x);
}
#endif