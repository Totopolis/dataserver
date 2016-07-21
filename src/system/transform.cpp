// transform.cpp
//
#include "common/common.h"
#include "transform.h"
#include "common/static_type.h"
#include "hilbert.inl"
#include "transform.inl"

namespace sdl { namespace db { namespace space { 

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
vector_type sort_unique(vector_type && result)
{
    if (!result.empty()) {
        std::sort(result.begin(), result.end());
        result.erase(std::unique(result.begin(), result.end()), result.end());
    }
    return std::move(result); // must return copy
}

template<class vector_type>
vector_type sort_unique(vector_type && v1, vector_type && v2)
{
    if (v1.empty()) {
        return sort_unique(std::move(v2));
    }
    if (v2.empty()) {
        return sort_unique(std::move(v1));
    }
    v1.insert(v1.end(), v2.begin(), v2.end());
    return sort_unique(std::move(v1));
}

//--------------------------------------------------------------------

struct math : is_static {
    enum { EARTH_ELLIPSOUD = false }; // to be tested
    enum quadrant {
        q_0 = 0, // [-45..45] longitude
        q_1 = 1, // (45..135]
        q_2 = 2, // (135..180][-180..-135)
        q_3 = 3  // [-135..-45)
    };
    enum class hemisphere {
        north = 0, // [0..90] latitude
        south = 1  // [-90..0)
    };
    struct sector {
        hemisphere h;
        quadrant q;
    };
    static hemisphere latitude_hemisphere(double const lat) {
        return (lat >= 0) ? hemisphere::north : hemisphere::south;
    }
    static hemisphere latitude_hemisphere(spatial_point const & s) {
        return latitude_hemisphere(s.latitude);
    }
    static hemisphere point_hemisphere(point_2D const & p) {
        return (p.Y >= 0.5) ? hemisphere::north : hemisphere::south;
    }
    static sector get_sector(spatial_point const &);
    static quadrant longitude_quadrant(double);
    static quadrant longitude_quadrant(Longitude);
    static double longitude_360(double);
    static bool cross_longitude(double, double, double);
    static double cross_quadrant(quadrant); // returns Longitude
    static bool is_cross_quadrant(spatial_rect const &);
    static double longitude_meridian(double, quadrant);
    static double reverse_longitude_meridian(double, quadrant);
    static point_3D cartesian(Latitude, Longitude);
    static spatial_point reverse_cartesian(point_3D const &);
    static point_3D line_plane_intersect(Latitude, Longitude);
    static point_2D scale_plane_intersect(const point_3D &, quadrant, hemisphere);
    static point_2D project_globe(spatial_point const &);
    static point_2D project_globe(Latitude const lat, Longitude const lon);
    static spatial_cell globe_to_cell(const point_2D &, spatial_grid);
    static double norm_longitude(double);
    static double norm_latitude(double);
    static double add_longitude(double const lon, double const d);
    static double add_latitude(double const lat, double const d);
    static double earth_radius(Latitude);
    static Meters haversine(spatial_point const & p1, spatial_point const & p2, Meters R);
    static Meters haversine(spatial_point const & p1, spatial_point const & p2);
    static spatial_point destination(spatial_point const &, Meters const distance, Degree const bearing);
    static point_XY<int> quadrant_grid(quadrant, int const grid);
    static point_XY<int> multiply_grid(point_XY<int> const & p, int const grid);
    static quadrant point_quadrant(point_2D const & p);
    static spatial_point reverse_line_plane_intersect(point_3D const &);
    static point_3D reverse_scale_plane_intersect(point_2D const &, quadrant, hemisphere);
    static spatial_point reverse_project_globe(point_2D const &);
    static bool destination_rect(spatial_rect &, spatial_point const &, Meters const radius);
    static vector_cell select_hemisphere(spatial_rect const &, spatial_grid);
private:
    static void interpolate_contour(vector_point_2D &, spatial_point const &, spatial_point const &);
    static vector_cell select_intersect(vector_point_2D const &, spatial_grid);
    static vector_cell select_sector(spatial_rect const &, spatial_grid);
    enum class contains_t {
        none,
        intersect,
        rect_inside,
        poly_inside
    };
    static contains_t contains(vector_point_2D const &, rect_2D const &);
private:
    static double earth_radius(Latitude const lat, bool_constant<true>);
    static double earth_radius(Latitude, bool_constant<false>) {
        return limits::EARTH_RADIUS;
    }
};

inline math::quadrant operator++(math::quadrant t) {
    return static_cast<math::quadrant>(static_cast<int>(t)+1);
}

inline bool operator == (math::sector const & x, math::sector const & y) { 
    return (x.h == y.h) && (x.q == y.q);
}
inline bool operator != (math::sector const & x, math::sector const & y) { 
    return !(x == y);
}
#if 0
inline bool operator < (math::sector const & x, math::sector const & y) {
    if (x.h == y.h) {
        return x.q < y.q;
    }
    return math::hemisphere::north == x.h;
}
#endif
// The shape of the Earth is well approximated by an oblate spheroid with a polar radius of 6357 km and an equatorial radius of 6378 km. 
// PROVIDED a spherical approximation is satisfactory, any value in that range will do, such as R (in km) = 6378 - 21 * sin(lat)
inline double math::earth_radius(Latitude const lat, bool_constant<true>) {
    constexpr double delta = limits::EARTH_MAJOR_RADIUS - limits::EARTH_MINOR_RADIUS;
    return limits::EARTH_MAJOR_RADIUS - delta * std::sin(a_abs(lat.value() * limits::DEG_TO_RAD));
}

inline double math::earth_radius(Latitude const lat) {
    return earth_radius(lat, bool_constant<EARTH_ELLIPSOUD>());
}

math::quadrant math::longitude_quadrant(double const x) {
    SDL_ASSERT(SP::valid_longitude(x));
    if (x >= 0) {
        if (x <= 45) return q_0;
        if (x <= 135) return q_1;
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

inline double math::longitude_360(double const d) {
    SDL_ASSERT(SP::valid_longitude(d));
    return (d < 0) ? (360 + d) : d;
}

bool math::cross_longitude(double const val, double const lon1, double const lon2) {
    double const mid = longitude_360(val);
    double const left = longitude_360(lon1);
    double const right = longitude_360(lon2);
    if (left <= right) {
        return (left < mid) && (mid < right);
    }
    return (mid < right) || (left < mid);
}

double math::cross_quadrant(quadrant const q) { // returns Longitude
    switch (q) {
    case q_0: return -45;
    case q_1: return 45;
    case q_2: return 135;
    default:
        SDL_ASSERT(q == q_3);
        return -135;
    }
}

inline math::sector math::get_sector(spatial_point const & p) {
    sector s;
    s.h = latitude_hemisphere(p.latitude);
    s.q = longitude_quadrant(p.longitude);
    return s;
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

point_2D math::project_globe(spatial_point const & s)
{
    SDL_ASSERT(s.is_valid());      
    const quadrant quad = longitude_quadrant(s.longitude);
    const double meridian = longitude_meridian(s.longitude, quad);
    SDL_ASSERT((meridian >= 0) && (meridian <= 90));    
    const bool is_north = (s.latitude >= 0);
    const point_3D p3 = line_plane_intersect(is_north ? s.latitude : -s.latitude, meridian);
    return scale_plane_intersect(p3, quad, is_north ? hemisphere::north : hemisphere::south);
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

inline point_XY<int> min_max(const point_2D & p, const int _max) {
    return{
        a_max<int>(a_min<int>(static_cast<int>(p.X), _max), 0),
        a_max<int>(a_min<int>(static_cast<int>(p.Y), _max), 0)
    };
};
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

} // namespace

/*struct rectangular {
    point_XY<int> h_0;
    point_XY<int> h_1;
    point_XY<int> h_2;
    point_XY<int> h_3;
};*/

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

// returns Z coordinate of vector multiplication
inline double rotate(double X1, double Y1, double X2, double Y2) {
    return X1 * Y2 - X2 * Y1;
}

// returns Z coordinate of vector multiplication
inline double rotate(point_2D const & p1, point_2D const & p2) {
    return p1.X * p2.Y - p2.X * p1.Y;
}

bool line_intersect(point_2D const & a, point_2D const & b, // line1 (a,b)
                    point_2D const & c, point_2D const & d) // line2 (c,d)
{
    const point_2D a_b = b - a;
    const point_2D c_d = d - c;
    return (fsign(rotate(a_b, c - b)) * fsign(rotate(a_b, d - b)) <= 0) &&
           (fsign(rotate(c_d, a - d)) * fsign(rotate(c_d, b - d)) <= 0);
}

bool line_rect_intersect(point_2D const & a, point_2D const & b, rect_2D const & rc)
{
    point_2D const & lb = rc.lb();
    if (line_intersect(a, b, rc.lt, lb)) return true; // => sign ?
    if (line_intersect(a, b, lb, rc.rb)) return true;
    point_2D const & rt = rc.rt();
    if (line_intersect(a, b, rc.rb, rt)) return true;
    if (line_intersect(a, b, rt, rc.lt)) return true;
    return false;
}

inline bool point_inside(point_2D const & p, rect_2D const & rc) {
    SDL_ASSERT(!(rc.rb < rc.lt));    
    return (p.X >= rc.lt.X) && (p.X <= rc.rb.X) &&
           (p.Y >= rc.lt.Y) && (p.Y <= rc.rb.Y);
}

#if 0
// https://en.wikipedia.org/wiki/Point_in_polygon 
// https://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
// Run a semi-infinite ray horizontally (increasing x, fixed y) out from the test point, and count how many edges it crosses. 
// At each crossing, the ray switches between inside and outside. This is called the Jordan curve theorem.
template<class float_>
int pnpoly(int const nvert,
           float_ const * const vertx, 
           float_ const * const verty,
           float_ const testx, 
           float_ const testy,
           float_ const epsilon = 0)
{
  int i, j, c = 0;
  for (i = 0, j = nvert-1; i < nvert; j = i++) {
    if ( ((verty[i]>testy) != (verty[j]>testy)) &&
     ((testx + epsilon) < (vertx[j]-vertx[i]) * (testy-verty[i]) / (verty[j]-verty[i]) + vertx[i]) )
       c = !c;
  }
  return c;
}
#endif

// https://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
inline bool ray_crossing(point_2D const & test, point_2D const & p1, point_2D const & p2) {
    return ((p1.Y > test.Y) != (p2.Y > test.Y)) &&
        ((test.X + limits::fepsilon) < ((test.Y - p2.Y) * (p1.X - p2.X) / (p1.Y - p2.Y) + p2.X));
}

math::contains_t
math::contains(vector_point_2D const & cont, rect_2D const & rc)
{
    SDL_ASSERT(!cont.empty());
    SDL_ASSERT(!(rc.rb < rc.lt));
    auto end = cont.end();
    auto first = end - 1;
    bool crossing = false;
    for (auto second = cont.begin(); second != end; ++second) {
        point_2D const & p1 = *first;   // vert[j]
        point_2D const & p2 = *second;  // vert[i]
        if (line_rect_intersect(p1, p2, rc)) {
            return contains_t::intersect;
        }
        if (ray_crossing(rc.lt, p1, p2)) {
            crossing = !crossing; // At each crossing, the ray switches between inside and outside
        }
        first = second;
    }
    // no intersection between contour and rect
    if (point_inside(cont[0], rc)) { // test any point of contour
        return contains_t::poly_inside;
    }
    if (crossing) {
        return contains_t::rect_inside;
    }
    return contains_t::none;
}

vector_cell math::select_intersect(vector_point_2D const & cont, spatial_grid const grid) // not optimized
{
    using namespace globe_to_cell_;

    SDL_ASSERT(cont.size() >= 4);

    rect_2D const bb = bound_box(cont.begin(), cont.end());

    SDL_ASSERT_1(frange(bb.lt.X, 0, 1));
    SDL_ASSERT_1(frange(bb.lt.Y, 0, 1));
    SDL_ASSERT_1(frange(bb.rb.X, 0, 1));
    SDL_ASSERT_1(frange(bb.rb.Y, 0, 1));

    const int g_0 = grid[0];
    const int g_1 = grid[1];
    const int g_2 = grid[2];
    const int g_3 = grid[3];

    const double f_0 = grid.f_0();
    const double f_1 = grid.f_1();
    const double f_2 = grid.f_2();
    const double f_3 = grid.f_3();

    const point_XY<int> lt_0 = min_max(scale(g_0, bb.lt), g_0 - 1);
    const point_XY<int> rb_0 = min_max(scale(g_0, bb.rb), g_0 - 1);

    rect_2D rc; // cell
    for (int x_0 = lt_0.X; x_0 <= rb_0.X; ++x_0) {
        rc.lt.X = x_0 * f_0;
        rc.rb.X = x_0 + rc.lt.X;
        for (int y_0 = lt_0.Y; y_0 <= rb_0.Y; ++y_0) {
            rc.lt.Y = y_0 * f_0;
            rc.rb.Y = y_0 + rc.lt.Y;
            if (contains(cont, rc) != contains_t::none) { //next level...
                //
            }
        }
    }
    vector_cell result;
    for (auto & p : cont) {
        result.push_back(globe_to_cell(p, grid)); // prototype
    }
    return sort_unique(std::move(result));
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

Meters math::haversine(spatial_point const & _1, spatial_point const & _2)
{
    const double R1 = earth_radius(_1.latitude);
    const double R2 = earth_radius(_2.latitude);
    const double R = (R1 + R2) / 2;
    return haversine(_1, _2, R);
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
    dest.longitude = norm_longitude(lon2 * limits::RAD_TO_DEG);
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
    return true;
}

bool math::is_cross_quadrant(spatial_rect const & rc) {
    for (size_t i = 0; i < 4; ++i) {
        if (cross_longitude(cross_quadrant(quadrant(i)), rc.min_lon, rc.max_lon)) {
            return true;
        }
    }
    return false;
}

void math::interpolate_contour(vector_point_2D & cont, spatial_point const & p1, spatial_point const & p2)
{
    SDL_ASSERT((p1.latitude == p2.latitude) || (p1.longitude == p2.longitude)); // expected
    cont.push_back(math::project_globe(p1));
    Meters const distance = math::haversine(p1, p2);
    size_t const num = a_max((size_t)(distance.value() / 100000), size_t(2)); // add contour point per each 100 km
    double const lat = (p2.latitude - p1.latitude) / (num + 1);
    double const lon = (p2.longitude - p1.longitude) / (num + 1);
    spatial_point mid;
    for (size_t i = 1; i <= num; ++i) {
        mid.latitude = p1.latitude + lat * i;
        mid.longitude = p1.longitude + lon * i;
        cont.push_back(math::project_globe(mid));
    }
}

vector_cell math::select_sector(spatial_rect const & rc, spatial_grid const grid)
{
    SDL_ASSERT(rc && !rc.cross_equator());
    SDL_ASSERT(!is_cross_quadrant(rc));
    vector_point_2D cont;
    for (size_t i = 0; i < 4; ++i) {
        interpolate_contour(cont, rc[i], rc[(i + 1) % 4]);
    }
    return select_intersect(cont, grid);
}

// note: may optimize interpolate_contour for common edges
vector_cell math::select_hemisphere(spatial_rect const & rc, spatial_grid const grid)
{
    SDL_ASSERT(rc && !rc.cross_equator());
    vector_cell result;
    spatial_rect sector = rc;
    for (size_t i = 0; i < 4; ++i) {
        double const d = math::cross_quadrant(math::quadrant(i));
        if (math::cross_longitude(d, sector.min_lon, sector.max_lon)) {
            sector.max_lon = d;
            result = sort_unique(std::move(result), select_sector(sector, grid));
            SDL_ASSERT(!result.empty());
            sector.min_lon = d;
            sector.max_lon = rc.max_lon;
        }
    }
    SDL_ASSERT(sector.max_lon == rc.max_lon);
    if (sector.min_lon == rc.min_lon) { // never crossed quadrant
        SDL_ASSERT(result.empty());
        return select_sector(rc, grid);
    }
    SDL_ASSERT(!result.empty());
    return sort_unique(std::move(result), math::select_sector(sector, grid));
}

#if 0 // reserved
math::sector const * find_not_equal(math::sector const * first, math::sector const * const end) {
    SDL_ASSERT(first < end);
    math::sector const & s = *(first++);
    for (; first != end; ++first) {
        if (s != *first)
            return first;
    }
    return nullptr;
}
#endif

} // namespace space

using namespace space;

spatial_cell transform::make_cell(spatial_point const & p, spatial_grid const g)
{
    return math::globe_to_cell(math::project_globe(p), g);
}

point_XY<int> transform::d2xy(spatial_cell::id_type const id, grid_size const size)
{
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
    if ((rc.min_lat < 0) && (0 < rc.max_lat)) { // cross equator
        spatial_rect r1 = rc;
        spatial_rect r2 = rc;
        r1.min_lat = 0; // [0..max_lat] north
        r2.max_lat = 0; // [min_lat..0] south
        return sort_unique(
            math::select_hemisphere(r1, grid),
            math::select_hemisphere(r2, grid));
    }
    return math::select_hemisphere(rc, grid);
}

vector_cell
transform::cell_range(spatial_point const & where, Meters const radius, spatial_grid const grid)
{
    point_2D const where_pos = math::project_globe(where);
    spatial_cell const where_cell = math::globe_to_cell(where_pos, grid);
    if (fless_eq(radius.value(), 0)) {
        return { where_cell };
    }
    spatial_rect rc;
    if (math::destination_rect(rc, where, radius)) { // temporal
        return cell_rect(rc, grid);
    }
    return{};
}

#if 0 // test
vector_cell
transform::cell_range(spatial_point const & where, Meters const radius, spatial_grid const grid)
{
    point_2D const where_pos = math::project_globe(where);
    spatial_cell const where_cell = math::globe_to_cell(where_pos, grid);
    if (fless_eq(radius.value(), 0)) {
        return { where_cell };
    }
    spatial_rect rc;
    if (!math::destination_rect(rc, where, radius)) {
        const math::hemisphere is_north = math::north_hemisphere(where);
        SDL_WARNING(0); // not implemented
        return {};
    }
    enum { size_4 = spatial_rect::size };
    point_2D p2[size_4];
    math::sector sec[size_4];
    for (size_t i = 0; i < size_4; ++i) {
        spatial_point const & s = rc[i];
        p2[i] = math::project_globe(s);
        sec[i] = math::get_sector(s);
    }
    auto const next = find_not_equal(sec, sec + size_4);
    if (!next) { // simple case
        rect_2D bound;
        get_bound(bound, p2, p2 + size_4);
        const double f_3 = grid.f_3(); // smallest step
        const size_t x1 = static_cast<size_t>(bound.lt.X / f_3);
        const size_t y1 = static_cast<size_t>(bound.lt.Y / f_3);
        const size_t x2 = static_cast<size_t>(bound.rb.X / f_3);
        const size_t y2 = static_cast<size_t>(bound.rb.Y / f_3);
        const size_t pos_x = static_cast<size_t>(where_pos.X / f_3);
        const size_t pos_y = static_cast<size_t>(where_pos.Y / f_3);
        SDL_ASSERT(x1 <= x2);
        SDL_ASSERT(y1 <= y2);
        SDL_ASSERT(x1 <= pos_x);
        SDL_ASSERT(y1 <= pos_y);
        if ((x1 == x2) && (y1 == y2)) {
            return { where_cell };
        }
        vector_cell result(1, where_cell);
        point_2D test;
        for (size_t x = x1; x <= x2; ++x) {
            test.X = x * f_3;
            for (size_t y = y1; y <= y2; ++y) {
                if ((x != pos_x) || (y != pos_y)) {
                    test.Y = y * f_3;
                    Meters const d = math::haversine(math::reverse_project_globe(test), where);
                    if (d.value() <= radius.value()) { // select cell at (x,y)
                        spatial_cell const it = math::globe_to_cell(test, grid);
                        SDL_ASSERT(cell_point(it, grid) == test);
                        result.push_back(it);
                    }
                }
            }
        }
        std::sort(result.begin(), result.end());
        auto last = std::unique(result.begin(), result.end());
        SDL_ASSERT(last == result.end()); // to be tested
        result.erase(last, result.end());
        return result;
    }
    else { // check each sector...
    }
    return{};
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
                        SDL_ASSERT_1(math::longitude_quadrant(45) == 0);
                        SDL_ASSERT_1(math::longitude_quadrant(90) == 1);
                        SDL_ASSERT_1(math::longitude_quadrant(135) == 1);
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
#endif
                    }
                    if (1)
                    {
                        spatial_cell x{}, y{};
                        SDL_ASSERT(spatial_cell::compare(x, y) == 0);
                        SDL_ASSERT(x == y);
                        y.set_depth(1);
                        SDL_ASSERT(x != y);
                        SDL_ASSERT(spatial_cell::compare(x, y) < 0);
                        SDL_ASSERT(spatial_cell::compare(y, x) > 0);
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
                        Meters const d1 = math::earth_radius(Latitude(0)) * limits::PI / 2;
                        Meters const d2 = d1.value() / 2;
                        SDL_ASSERT(math::destination(SP::init(Latitude(0), Longitude(0)), d1, Degree(0)).equal(Latitude(90), Longitude(0)));
                        SDL_ASSERT(math::destination(SP::init(Latitude(0), Longitude(0)), d1, Degree(360)).equal(Latitude(90), Longitude(0)));
                        SDL_ASSERT(math::destination(SP::init(Latitude(0), Longitude(0)), d2, Degree(0)).equal(Latitude(45), Longitude(0)));
                        SDL_ASSERT(math::destination(SP::init(Latitude(0), Longitude(0)), d2, Degree(90)).equal(Latitude(0), Longitude(45)));
                        SDL_ASSERT(math::destination(SP::init(Latitude(0), Longitude(0)), d2, Degree(180)).equal(Latitude(-45), Longitude(0)));
                        SDL_ASSERT(math::destination(SP::init(Latitude(0), Longitude(0)), d2, Degree(270)).equal(Latitude(0), Longitude(-45)));
                        SDL_ASSERT(math::destination(SP::init(Latitude(90), Longitude(0)), d2, Degree(0)).equal(Latitude(45), Longitude(0)));
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
