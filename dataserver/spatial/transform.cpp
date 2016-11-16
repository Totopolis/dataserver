// transform.cpp
//
#include "dataserver/spatial/transform.h"
#include "dataserver/spatial/hilbert.inl"
#include "dataserver/spatial/transform.inl"
#include "dataserver/system/page_info.h"
#include "dataserver/common/static_type.h"
#include "dataserver/common/array.h"
#include <iomanip> // for std::setprecision

namespace sdl { namespace db { namespace space { 

#if SDL_DEBUG
template<class T>
void debug_trace(T const & v) {
    size_t i = 0;
    for (auto & p : v) {
        std::cout << (i++)
            << std::setprecision(9)
            << "," << p.X
            << "," << p.Y
            << "\n";
    }
}

#if SDL_USE_INTERVAL_CELL
void debug_trace(interval_cell const & v){
    size_t i = 0;
    v.for_each([&i](interval_cell::cell_ref cell){
        point_2D const p = transform::cell2point(cell);
        spatial_point const sp = transform::spatial(cell);
        std::cout << (i++)
            << std::setprecision(9)
            << "," << p.X
            << "," << p.Y
            << "," << sp.longitude
            << "," << sp.latitude
            << "\n";
        return bc::continue_;
    });
}
#endif // #if SDL_USE_INTERVAL_CELL
#endif // #if SDL_DEBUG

#define USE_EARTH_ELLIPSOUD     0

struct math : is_static {
    enum { EARTH_ELLIPSOUD = 0 }; // to be tested
    enum quadrant {
        q_0 = 0, // [-45..45) longitude
        q_1 = 1, // [45..135)
        q_2 = 2, // [135..180][-180..-135)
        q_3 = 3  // [-135..-45)
    };
    enum { quadrant_size = 4 };
    enum class hemisphere {
        north = 0, // [0..90] latitude
        south = 1  // [-90..0)
    };
    struct sector_t {
        hemisphere h;
        quadrant q;
    };
    struct sector_index {
        sector_t sector;
        size_t index;
    };
    using function_ref = transform::function_ref;
    using buf_sector = vector_buf<sector_index, 4>;
    using buf_XY = vector_buf<XY, 36>;
    using buf_2D = vector_buf<point_2D, 36>;
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
    static bool is_pole(spatial_point const & p) {
        return latitude_pole(p.latitude);
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
    static spatial_cell globe_to_cell(const point_2D &, spatial_grid);
    static spatial_cell globe_make_cell(spatial_point const &, spatial_grid);
    static double norm_longitude(double);
    static double norm_latitude(double);
    static double add_longitude(double const lon, double const d);
    static double add_latitude(double const lat, double const d);
    static Meters spherical_cosines(spatial_point const &, spatial_point const &);
    static Meters haversine(spatial_point const &, spatial_point const &);
    static Meters haversine_error(spatial_point const &, spatial_point const &, Meters);
    static spatial_point destination(spatial_point const &, Meters const distance, Degree const bearing);
    static Degree course_between_points(spatial_point const &, spatial_point const &);
    static Meters cross_track_distance(spatial_point const &, spatial_point const &, spatial_point const &);
    static Meters track_distance(spatial_point const *, spatial_point const *, spatial_point const &, spatial_rect const * bbox);
    static Meters track_distance(spatial_point const *, spatial_point const *, spatial_point const &);
    static Meters min_distance(spatial_point const *, spatial_point const *, spatial_point const &);
#if 0 //SDL_DEBUG
    static Meters poly_distance(spatial_point const * first1, spatial_point const * last1,
                                     spatial_point const * first2, spatial_point const * last2);
#endif
    static point_XY<int> quadrant_grid(quadrant, int const grid);
    static point_XY<int> multiply_grid(point_XY<int> const & p, int const grid);
    static quadrant point_quadrant(point_2D const & p);
    static spatial_point reverse_line_plane_intersect(point_3D const &);
    static point_3D reverse_scale_plane_intersect(point_2D const &, quadrant, hemisphere);
    static spatial_point reverse_project_globe(point_2D const &);
    static void poly_latitude(buf_2D &, double const lat, double const lon1, double const lon2, hemisphere, bool);
    static void poly_longitude(buf_2D &, double const lon, double const lat1, double const lat2, hemisphere, bool);
    static void poly_rect(buf_2D & verts, spatial_rect const &, hemisphere);
    static void poly_range(buf_sector &, buf_2D & verts, spatial_point const &, Meters, sector_t const &, spatial_grid);
    static break_or_continue fill_poly(function_ref, point_2D const *, point_2D const *, spatial_grid);
    static break_or_continue fill_poly(function_ref, buf_2D const &, spatial_grid);
    static break_or_continue select_hemisphere(function_ref, spatial_rect const &, spatial_grid);
    static break_or_continue select_sector(function_ref, spatial_rect const &, spatial_grid);    
    static break_or_continue select_range(function_ref, spatial_point const &, Meters, spatial_grid);
private: 
    static spatial_cell make_cell_depth_1(XY const &, spatial_grid);
    static spatial_cell make_cell_depth_2(XY const &, spatial_grid);
    static spatial_cell make_cell_depth_3(XY const &, spatial_grid);
    static spatial_cell make_cell_depth_4(XY const &, spatial_grid);
public: 
    using scan_lines_int = std::vector<vector_buf<int, 4>>;
    static break_or_continue fill_internal(function_ref, scan_lines_int const &, rect_XY const &, spatial_grid);
private: 
#if USE_EARTH_ELLIPSOUD // to be tested
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
    static double earth_radius(Latitude const lat) {
        return earth_radius(lat.value(), bool_constant<EARTH_ELLIPSOUD>());
    }
    static double earth_radius(Latitude const lat1, Latitude const lat2) {
        return earth_radius(lat1.value(), lat2.value(), bool_constant<EARTH_ELLIPSOUD>());
    }
    static double earth_radius(spatial_point const & p1, spatial_point const & p2) {
        return earth_radius(p1.latitude, p2.latitude);
    }
    static double earth_radius(spatial_point const & p) {
        return earth_radius(p.latitude);
    }
#endif
public:
    static const double order_quadrant[quadrant_size];
    static const double sorted_quadrant[quadrant_size]; // longitudes
    static const point_2D north_quadrant[quadrant_size];
    static const point_2D south_quadrant[quadrant_size];
    static const point_2D pole_hemisphere[2];
    static const point_2D & sector_point(sector_t);
};

const double math::order_quadrant[quadrant_size] = { -45, 45, 135, -135 };
const double math::sorted_quadrant[quadrant_size] = { -135, -45, 45, 135 };

const point_2D math::north_quadrant[quadrant_size] = {
    { 1, 0.5 },
    { 1, 1 },  
    { 0, 1 },  
    { 0, 0.5 } 
};
const point_2D math::south_quadrant[quadrant_size] = {
    { 1, 0.5 },
    { 1, 0 },
    { 0, 0 },
    { 0, 0.5 } 
};

const point_2D math::pole_hemisphere[2] = {
    { 0.5, 0.75 }, // north
    { 0.5, 0.25 }  // south
};

//-------------------------------------------------------------------

inline const point_2D & math::sector_point(sector_t const s) {
    return (s.h == hemisphere::north) ? north_quadrant[s.q] : south_quadrant[s.q];
}

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

#if USE_EARTH_ELLIPSOUD
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
#endif

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
#if SDL_DEBUG
    static const point_3D P0 { 0, 0, 0 };
    static const point_3D V0 { 1, 0, 0 };
    static const point_3D e2 { 0, 1, 0 };
    static const point_3D e3 { 0, 0, 1 };
#endif
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

spatial_cell math::make_cell_depth_1(XY const & p_0, spatial_grid const grid)
{
    SDL_ASSERT(p_0.X >= 0);
    SDL_ASSERT(p_0.Y >= 0);
    SDL_ASSERT(p_0.X < grid.s_0());
    SDL_ASSERT(p_0.Y < grid.s_0());
    //FIXME: assert mod_XY
    spatial_cell cell{};
    cell.set_id<0>(hilbert::s_xy2d<spatial_cell::id_type>(p_0));
    cell.data.depth = 1;
    return cell;
}

spatial_cell math::make_cell_depth_2(XY const & p_0, spatial_grid const grid)
{
    using namespace globe_to_cell_;
    SDL_ASSERT(p_0.X >= 0);
    SDL_ASSERT(p_0.Y >= 0);
    SDL_ASSERT(p_0.X < grid.s_1());
    SDL_ASSERT(p_0.Y < grid.s_1());
    const XY h_0 = div_XY<grid.s_0()>(p_0);
    const XY h_1 = mod_XY<grid.s_0()>(p_0, h_0);
    SDL_ASSERT((h_0.X >= 0) && (h_0.X < grid[0]));
    SDL_ASSERT((h_0.Y >= 0) && (h_0.Y < grid[0]));
    SDL_ASSERT((h_1.X >= 0) && (h_1.X < grid[1]));
    SDL_ASSERT((h_1.Y >= 0) && (h_1.Y < grid[1]));
    spatial_cell cell{};
    cell.set_id<0>(hilbert::s_xy2d<spatial_cell::id_type>(h_0)); // hilbert curve distance 
    cell.set_id<1>(hilbert::s_xy2d<spatial_cell::id_type>(h_1));
    cell.data.depth = 2;
    return cell;
}

spatial_cell math::make_cell_depth_3(XY const & p_0, spatial_grid const grid)
{
    using namespace globe_to_cell_;
    SDL_ASSERT(p_0.X >= 0);
    SDL_ASSERT(p_0.Y >= 0);
    SDL_ASSERT(p_0.X < grid.s_2());
    SDL_ASSERT(p_0.Y < grid.s_2());
    const XY h_0 = div_XY<grid.s_1()>(p_0);
    const XY p_1 = mod_XY<grid.s_1()>(p_0, h_0);
    const XY h_1 = div_XY<grid.s_0()>(p_1);
    const XY h_2 = mod_XY<grid.s_0()>(p_1, h_1);
    SDL_ASSERT((h_0.X >= 0) && (h_0.X < grid[0]));
    SDL_ASSERT((h_0.Y >= 0) && (h_0.Y < grid[0]));
    SDL_ASSERT((h_1.X >= 0) && (h_1.X < grid[1]));
    SDL_ASSERT((h_1.Y >= 0) && (h_1.Y < grid[1]));
    SDL_ASSERT((h_2.X >= 0) && (h_2.X < grid[2]));
    SDL_ASSERT((h_2.Y >= 0) && (h_2.Y < grid[2]));
    spatial_cell cell{};
    cell.set_id<0>(hilbert::s_xy2d<spatial_cell::id_type>(h_0)); // hilbert curve distance 
    cell.set_id<1>(hilbert::s_xy2d<spatial_cell::id_type>(h_1));
    cell.set_id<2>(hilbert::s_xy2d<spatial_cell::id_type>(h_2));
    cell.data.depth = 3;
    return cell;
}

#if high_grid_optimization
inline spatial_cell math::make_cell_depth_4(XY const & p_0, spatial_grid const grid)
{
    using namespace globe_to_cell_;
    SDL_ASSERT(p_0.X >= 0);
    SDL_ASSERT(p_0.Y >= 0);
    SDL_ASSERT(p_0.X < grid.s_3());
    SDL_ASSERT(p_0.Y < grid.s_3());
    const XY h_0 = div_XY<grid.s_2()>(p_0);
    const XY p_1 = mod_XY<grid.s_2()>(p_0, h_0);
    const XY h_1 = div_XY<grid.s_1()>(p_1);
    const XY p_2 = mod_XY<grid.s_1()>(p_1, h_1);
    const XY h_2 = div_XY<grid.s_0()>(p_2);
    const XY h_3 = mod_XY<grid.s_0()>(p_2, h_2);
    SDL_ASSERT((h_0.X >= 0) && (h_0.X < grid[0]));
    SDL_ASSERT((h_0.Y >= 0) && (h_0.Y < grid[0]));
    SDL_ASSERT((h_1.X >= 0) && (h_1.X < grid[1]));
    SDL_ASSERT((h_1.Y >= 0) && (h_1.Y < grid[1]));
    SDL_ASSERT((h_2.X >= 0) && (h_2.X < grid[2]));
    SDL_ASSERT((h_2.Y >= 0) && (h_2.Y < grid[2]));
    SDL_ASSERT((h_3.X >= 0) && (h_3.X < grid[3]));
    SDL_ASSERT((h_3.Y >= 0) && (h_3.Y < grid[3]));
    spatial_cell cell; // uninitialized
    cell.set_id<0>(hilbert::s_xy2d<spatial_cell::id_type>(h_0)); // hilbert curve distance 
    cell.set_id<1>(hilbert::s_xy2d<spatial_cell::id_type>(h_1));
    cell.set_id<2>(hilbert::s_xy2d<spatial_cell::id_type>(h_2));
    cell.set_id<3>(hilbert::s_xy2d<spatial_cell::id_type>(h_3));
    cell.data.depth = 4;
    return cell;
}
#else
spatial_cell math::make_cell_depth_4(XY const & p_0, spatial_grid const grid)
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
    cell.set_id<0>(hilbert::xy2d<spatial_cell::id_type>(grid[0], h_0)); // hilbert curve distance 
    cell.set_id<1>(hilbert::xy2d<spatial_cell::id_type>(grid[1], h_1));
    cell.set_id<2>(hilbert::xy2d<spatial_cell::id_type>(grid[2], h_2));
    cell.set_id<3>(hilbert::xy2d<spatial_cell::id_type>(grid[3], h_3));
    cell.data.depth = 4;
    return cell;
}
#endif

#if high_grid_optimization
spatial_cell math::globe_to_cell(const point_2D & globe, spatial_grid const grid)
{
    using namespace globe_to_cell_;

    enum { g_0 = spatial_grid::get<0>() };
    enum { g_1 = spatial_grid::get<1>() };
    enum { g_2 = spatial_grid::get<2>() };
    enum { g_3 = spatial_grid::get<3>() };

    SDL_ASSERT_1(frange(globe.X, 0, 1));
    SDL_ASSERT_1(frange(globe.Y, 0, 1));

    const point_XY<int> h_0 = min_max<g_0 - 1>(scale<g_0>(globe));
    const point_2D fraction_0 = fraction<g_0>(globe, h_0);

    SDL_ASSERT_1(frange(fraction_0.X, 0, 1));
    SDL_ASSERT_1(frange(fraction_0.Y, 0, 1));

    const point_XY<int> h_1 = min_max<g_1 - 1>(scale<g_1>(fraction_0));
    const point_2D fraction_1 = fraction<g_1>(fraction_0, h_1);

    SDL_ASSERT_1(frange(fraction_1.X, 0, 1));
    SDL_ASSERT_1(frange(fraction_1.Y, 0, 1));

    const point_XY<int> h_2 = min_max<g_2 - 1>(scale<g_2>(fraction_1));
    const point_2D fraction_2 = fraction<g_2>(fraction_1, h_2);

    SDL_ASSERT_1(frange(fraction_2.X, 0, 1));
    SDL_ASSERT_1(frange(fraction_2.Y, 0, 1));

    const point_XY<int> h_3 = min_max<g_3 - 1>(scale<g_3>(fraction_2));
    spatial_cell cell; // uninitialized
    cell.set_id<0>(hilbert::n_xy2d<g_0, spatial_cell::id_type>(h_0)); // hilbert curve distance 
    cell.set_id<1>(hilbert::n_xy2d<g_1, spatial_cell::id_type>(h_1));
    cell.set_id<2>(hilbert::n_xy2d<g_2, spatial_cell::id_type>(h_2));
    cell.set_id<3>(hilbert::n_xy2d<g_3, spatial_cell::id_type>(h_3));
    cell.data.depth = 4;
    return cell;
}
#else
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
    cell.set_id<0>(hilbert::xy2d<spatial_cell::id_type>(g_0, h_0)); // hilbert curve distance 
    cell.set_id<1>(hilbert::xy2d<spatial_cell::id_type>(g_1, h_1));
    cell.set_id<2>(hilbert::xy2d<spatial_cell::id_type>(g_2, h_2));
    cell.set_id<3>(hilbert::xy2d<spatial_cell::id_type>(g_3, h_3));
    cell.data.depth = 4;
    return cell;
}
#endif

inline spatial_cell math::globe_make_cell(spatial_point const & p, spatial_grid const g) {
    return globe_to_cell(project_globe(p), g);
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
Meters math::haversine(spatial_point const & p1, spatial_point const & p2)
{
    const double lat1 = limits::DEG_TO_RAD * p1.latitude;
    const double lat2 = limits::DEG_TO_RAD * p2.latitude;
    const double dlon = limits::DEG_TO_RAD * (p2.longitude - p1.longitude);
    const double dlat = lat2 - lat1;
    const double sin_dlat = sin(dlat * 0.5);
    const double sin_dlon = sin(dlon * 0.5);
    const double a = sin_dlat * sin_dlat + cos(lat1) * cos(lat2) * sin_dlon * sin_dlon;
    return 2.0 * asin(a_min(sqrt(a), 1.0)) * limits::EARTH_RADIUS;
}

inline Meters math::haversine_error(spatial_point const & p1, spatial_point const & p2, Meters const radius)
{
    return a_abs(haversine(p1, p2).value() - radius.value());
}

// https://en.wikipedia.org/wiki/Spherical_law_of_cosines
Meters math::spherical_cosines(spatial_point const & p1, spatial_point const & p2)
{
    const double lat1 = limits::DEG_TO_RAD * p1.latitude;
    const double lat2 = limits::DEG_TO_RAD * p2.latitude;
    const double dlon = limits::DEG_TO_RAD * (p2.longitude - p1.longitude);
    return acos(sin(lat1) * sin(lat2) + cos(lat1) * cos(lat2) * cos(dlon)) * limits::EARTH_RADIUS;
}

/*
http://www.movable-type.co.uk/scripts/latlong.html
http://williams.best.vwh.net/avform.htm#LL
Destination point given distance and bearing from start point
Given a start point, initial bearing, and distance, this will calculate 
the destination point and final bearing travelling along a (shortest distance) great circle arc. */
spatial_point math::destination(spatial_point const & p, Meters const distance, Degree const bearing)
{
    SDL_ASSERT(frange(bearing.value(), 0, 360)); // clockwize direction to north [0..360]
    if (distance.value() <= 0) {
        return p;
    }
    const double dist = distance.value() / limits::EARTH_RADIUS; // angular distance in radians
    const double brng = bearing.value() * limits::DEG_TO_RAD;
    const double lat1 = p.latitude * limits::DEG_TO_RAD;
    const double cos_dist = std::cos(dist);
    const double sin_dist = std::sin(dist);
    const double sin_lat1 = std::sin(lat1);
    const double cos_lat1 = std::cos(lat1);
    const double lat2 = std::asin(sin_lat1 * cos_dist + cos_lat1 * sin_dist * std::cos(brng));
    const double x = cos_dist - sin_lat1 * std::sin(lat2);
    const double y = std::sin(brng) * sin_dist * cos_lat1;
    const double lon2 = (p.longitude * limits::DEG_TO_RAD) + fatan2(y, x); // lon1 = p.longitude * limits::DEG_TO_RAD
    spatial_point dest;
    dest.latitude = norm_latitude(lat2 * limits::RAD_TO_DEG);
    dest.longitude = norm_longitude(is_pole(p) ? bearing.value() : (lon2 * limits::RAD_TO_DEG));
    SDL_ASSERT(dest.is_valid());
    return dest;
}

Degree math::course_between_points(spatial_point const & p1, spatial_point const & p2)
{
    if (is_pole(p1)) {
        return (p1.latitude > 0) ? Degree(180) : Degree(0); // north : south pole
    }
    else {
        const double lon12 = (p1.longitude - p2.longitude) * limits::DEG_TO_RAD;
        const double lat1 = p1.latitude * limits::DEG_TO_RAD;
        const double lat2 = p2.latitude * limits::DEG_TO_RAD;
        const double cos_lat2 = cos(lat2);
        const double atan_y = sin(lon12) * cos_lat2;
        const double atan_x = cos(lat1) * sin(lat2) - sin(lat1) * cos_lat2 * cos(lon12);
        double degree = - fatan2(atan_y, atan_x) * limits::RAD_TO_DEG;
        if (fzero(degree))
            return 0;
        if (degree < 0)
            degree += 360; // normalize [0..360]
        SDL_ASSERT(frange(degree, 0, 360));
        SDL_ASSERT(!fequal(degree, 360));
        return degree;
    }
}

//http://stackoverflow.com/questions/32771458/distance-from-lat-lng-point-to-minor-arc-segment
Meters math::cross_track_distance(spatial_point const & A, 
                                  spatial_point const & B,
                                  spatial_point const & D)
{
    if (is_pole(A)) {
        return haversine(D, spatial_point::init(Latitude(D.latitude), Longitude(B.longitude)));
    }
    const double angle = a_abs(
        course_between_points(A, D).value() - 
        course_between_points(A, B).value()); //FIXME: can be optimized
    if (angle > 180) {
        if ((360 - 90) > angle) { // relative bearing is obtuse, if ((360 - angle) > 90)
            return haversine(A, D);
        }        
    }
    else if (angle > 90) { // relative bearing is obtuse
        return haversine(A, D);
    }
    const double angular_dist = haversine(A, D).value() / limits::EARTH_RADIUS;
    const double XTD = a_abs(asin(sin(angular_dist) * sin(angle * limits::DEG_TO_RAD))); // cross track error (distance off course) 
    const double ATD = a_abs(acos(cos(angular_dist) / cos(XTD))) * limits::EARTH_RADIUS; // along track distance
    if (fless_eq(haversine(A, B).value(), ATD)) {
        return haversine(B, D);
    }
    return XTD * limits::EARTH_RADIUS;
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
    const double arg = (p.Y >= 0.5) ? // is_north ?
        polar_2D::polar_arg(p.X - 0.5, p.Y - 0.75) : // pole{ 0.5, is_north ? 0.75 : 0.25 };
        -polar_2D::polar_arg(p.X - 0.5, p.Y - 0.25);
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

bool math::rect_cross_quadrant(spatial_rect const & rc) {
    for (size_t i = 0; i < quadrant_size; ++i) {
        if (cross_longitude(sorted_quadrant[i], rc.min_lon, rc.max_lon)) {
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

void math::poly_latitude(buf_2D & dest,
                        double const lat, 
                        double const _lon1,
                        double const _lon2,
                        hemisphere const h,
                        bool const change_direction) { // with first and last points
    SDL_ASSERT(_lon1 != _lon2);
    double const lon1 = change_direction ? _lon2 : _lon1;
    double const lon2 = change_direction ? _lon1 : _lon2;
    double const ld = longitude_distance(_lon1, _lon2, change_direction);
    SDL_ASSERT_1(change_direction ? frange(ld, -90, 0) : frange(ld, 0, 90));
    spatial_point const p1 = SP::init(Latitude(lat), Longitude(lon1));
    spatial_point const p2 = SP::init(Latitude(lat), Longitude(lon2));
    enum { min_num = 3 }; // must be odd    
    size_t const num = min_num + static_cast<size_t>(math::haversine(p1, p2).value() / 100000) * 2; //FIXME: experimental, must be odd
    double const step = ld / (num + 1);
    dest.push_back(project_globe(p1, h));
    SP mid;
    mid.latitude = lat;
    for (size_t i = 1; i <= num; ++i) {
        mid.longitude = add_longitude(lon1, step * i);
        dest.push_back(project_globe(mid, h));
    }
    dest.push_back(project_globe(p2, h));
}

void math::poly_longitude(buf_2D & dest,
                        double const lon, 
                        double const _lat1,
                        double const _lat2,
                        hemisphere const h,
                        bool const change_direction) { // without first and last points
    SDL_ASSERT(_lat1 != _lat2);
    double const lat1 = change_direction ? _lat2 : _lat1;
    double const lat2 = change_direction ? _lat1 : _lat2;
    double const ld = lat2 - lat1;
    spatial_point const p1 = SP::init(Latitude(lat1), Longitude(lon));
    spatial_point const p2 = SP::init(Latitude(lat2), Longitude(lon));
    enum { min_num = 3 }; // must be odd    
    size_t const num = min_num + static_cast<size_t>(math::haversine(p1, p2).value() / 100000) * 2; //FIXME: experimental, must be odd
    double const step = ld / (num + 1);
    SP mid;
    mid.longitude = lon;
    for (size_t i = 1; i <= num; ++i) {
        mid.latitude = lat1 + step * i;
        dest.push_back(project_globe(mid, h));
    }
}

inline void math::poly_rect(buf_2D & dest, spatial_rect const & rc, hemisphere const h) {
    poly_latitude(dest, rc.min_lat, rc.min_lon, rc.max_lon, h, false);
    poly_latitude(dest, rc.max_lat, rc.min_lon, rc.max_lon, h, true);
}

break_or_continue
math::select_sector(function_ref result, spatial_rect const & rc, spatial_grid const grid)
{
    SDL_ASSERT(rc && !rc.cross_equator() && !rect_cross_quadrant(rc));
    SDL_ASSERT(fless_eq(longitude_distance(rc.min_lon, rc.max_lon), 90));
    SDL_ASSERT(longitude_quadrant(rc.min_lon) <= longitude_quadrant(rc.max_lon));
    buf_2D verts;
    poly_rect(verts, rc, latitude_hemisphere((rc.min_lat + rc.max_lat) / 2));
    return fill_poly(result, verts, grid);
}

break_or_continue
math::select_hemisphere(function_ref result, spatial_rect const & rc, spatial_grid const grid)
{
    SDL_ASSERT(rc && !rc.cross_equator());
    spatial_rect sector = rc;
    for (size_t i = 0; i < quadrant_size; ++i) {
        double const d = math::sorted_quadrant[i];
        SDL_ASSERT((0 == i) || (math::sorted_quadrant[i - 1] < d));
        if (cross_longitude(d, sector.min_lon, sector.max_lon)) {
            SDL_ASSERT(d != sector.min_lon);
            SDL_ASSERT(d != sector.max_lon);
            sector.max_lon = d;
            if (is_break(select_sector(result, sector, grid))) {
                return bc::break_;
            }
            sector.min_lon = d;
            sector.max_lon = rc.max_lon;
        }
    }
    SDL_ASSERT(sector && (sector.max_lon == rc.max_lon));
    return select_sector(result, sector, grid);
}

template<class fun_type>
spatial_point intersection(
                    spatial_point const & where,
                    Meters const radius,
                    double bear1,
                    double bear2,
                    fun_type const & compare)
{
    SDL_ASSERT(bear1 < bear2);
    enum { max_count = 10 };
    spatial_point mid;
    size_t count = 0;
    for (; count < max_count; ++count) {
        const double bearing = (bear1 + bear2) * 0.5;
        mid = math::destination(where, radius, bearing);
        if (auto err = compare(mid)) {
            if (err < 0)
                bear2 = bearing;
            else
                bear1 = bearing;
        }
        else
            break;
    }
    SDL_ASSERT(count < max_count); // not enough iterations ?
    return mid;
}

void math::poly_range(buf_sector & cross, buf_2D & result, 
                      spatial_point const & where, Meters const radius, 
                      sector_t const & where_sec, spatial_grid const grid)
{
    SDL_ASSERT(radius.value() > 0);
    SDL_ASSERT(where_sec == spatial_sector(where));
    SDL_ASSERT(result.empty());
    SDL_ASSERT(cross.empty());

    enum { meter_error = 5 };
    enum { min_num = 32 };
    const double degree = limits::RAD_TO_DEG * radius.value() / limits::EARTH_RADIUS;
    const size_t num = globe_to_cell_::roundup<min_num>(degree * 32); //FIXME: experimental
    SDL_ASSERT(num && !(num % min_num));
    const double bx = 360.0 / num;
    SDL_ASSERT(frange(bx, 1.0, 360.0 / min_num));

    spatial_point sp = destination(where, radius, Degree(0)); // bearing = 0
    sector_t sec1 = spatial_sector(sp), sec2;
    result.push_back(project_globe(sp));
    point_2D next;
    for (double bearing = bx; bearing < 360; bearing += bx) {
        sp = destination(where, radius, Degree(bearing));
        next = project_globe(sp);
        if ((sec2 = spatial_sector(sp)) != sec1) {
            if (sec1.h != sec2.h) { // find intersection with equator
                static_assert(hemisphere::north < hemisphere::south, "");
                bool const north_to_south = sec1.h < sec2.h;
                const spatial_point mid = intersection(where, radius, bearing - bx, bearing,
                    [&where, radius, north_to_south](spatial_point & mid) {
                    const int ret = mid.latitude > 0 ? 1 : -1;
                    mid.latitude = 0;
                    const double meter = haversine_error(where, mid, radius).value();
                    if (meter < meter_error) {
                        return 0;
                    }
                    return north_to_south ? ret : -ret;
                });
                SDL_ASSERT(0 == mid.latitude);
                cross.emplace_back(sec2, result.size());
                result.push_back(project_globe(mid, sec1.h));
                result.push_back(project_globe(mid, sec2.h));
            }
            else { // intersection with quadrant
                result.push_back(project_globe(destination(where, radius, Degree(bearing - bx * 0.5)), sec1.h));
            }
            sec1 = sec2;
        }
        result.push_back(next);
    }
}

#if SDL_DEBUG > 1 // code sample
void debug_fill_poly_v2i_n(
        const int xmin, const int ymin, const int xmax, const int ymax,
        const int verts[][2], const int nr,
        void (*callback)(int, int, void *), void *userData)
{
	/* originally by Darel Rex Finley, 2007 */

	int  nodes, pixel_y, i, j;
	int *node_x = (int *) std::malloc(sizeof(*node_x) * (size_t)(nr + 1));

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
				std::swap(node_x[i], node_x[i + 1]);
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
	std::free(node_x);
}
/* https://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
inline bool ray_crossing(point_2D const & test, point_2D const & p1, point_2D const & p2) {
    return ((p1.Y > test.Y) != (p2.Y > test.Y)) &&
        ((test.X + limits::fepsilon) < ((test.Y - p2.Y) * (p1.X - p2.X) / (p1.Y - p2.Y) + p2.X));
}
*/

template<class fun_type>
void plot_line(point_2D const & p1, point_2D const & p2, const int max_id, fun_type set_pixel)
{
    //http://members.chello.at/~easyfilter/bresenham.c

    using namespace globe_to_cell_;    
    int x0 = min_max(max_id * p1.X, max_id - 1);
    int y0 = min_max(max_id * p1.Y, max_id - 1);
    const int x1 = min_max(max_id * p2.X, max_id - 1);
    const int y1 = min_max(max_id * p2.Y, max_id - 1);   
    const int dx = a_abs(x1 - x0);
    const int dy = -a_abs(y1 - y0);
    const int sx = (x0 < x1) ? 1 : -1;
    const int sy = (y0 < y1) ? 1 : -1;    
    int err = dx + dy, e2;  // error value e_xy

    for (;;) { // not including last point
        set_pixel(x0, y0);
        e2 = 2 * err;                                   
        if (e2 >= dy) {             // e_xy + e_x > 0
            if (x0 == x1) break;                       
            err += dy; x0 += sx;                       
        }
        if (e2 <= dx) {             // e_xy + e_y < 0
            if (y0 == y1) break;
            err += dx; y0 += sy;
        }
    }        
}
#endif // code sample

#if SDL_DEBUG
namespace fill_internal_ {
    template<class T, typename coord_y, class fun_type>
    void scan_lines_for_pair(T const & scan_lines, coord_y Y, fun_type && fun) {
        A_STATIC_ASSERT_TYPE(int, coord_y);
        for (auto const & line_x : scan_lines) {
            SDL_ASSERT(std::is_sorted(line_x.cbegin(), line_x.cend()));
            const size_t size = line_x.size();
            SDL_ASSERT(!is_odd(size));
            if (size > 1) {
                const auto * p = line_x.data();
                const auto * const last = p + size - 1;
                while (p < last) {
                    auto const x1 = *p++;
                    auto const x2 = *p++;
                    fun(Y, x1, x2);
                }
            }
            ++Y;
        }
    }
    template<class T>
    void trace_scan_lines(T const & data) {
        int y = 0;
        for (auto const & d : data) {
            for (auto const & x : d) {
                SDL_TRACE(x, ",", y);
            }
            ++y;
        }
    }

} // fill_internal_
#endif

break_or_continue
math::fill_internal(function_ref result,
                    scan_lines_int const & scan_lines_4, 
                    rect_XY const & bbox, 
                    spatial_grid const grid)
{
    enum { t_0 = spatial_grid::s_0() }; // top down
    enum { t_1 = spatial_grid::s_1() };
    enum { t_2 = spatial_grid::s_2() };
    enum { t_3 = spatial_grid::s_3() };

    enum { b_0 = spatial_grid::b_0() }; // bottom up
    enum { b_1 = spatial_grid::b_1() };
    enum { b_2 = spatial_grid::b_2() };
    enum { b_3 = spatial_grid::b_3() };

    static_assert(t_0 == 16, "");
    static_assert(t_1 == 256, "");
    static_assert(t_2 == 4096, "");
    static_assert(t_3 == 65536, "");

    static_assert(b_3 == 16, "");
    static_assert(b_2 == 256, "");
    static_assert(b_1 == 4096, "");
    static_assert(b_0 == 65536, "");

    enum { margin1 = 1 };

    SDL_ASSERT(bbox.is_valid());
    const size_t width_3 = (bbox.right() / b_3) - (bbox.left() / b_3);
    if (width_3 < 2) {
        XY fill = bbox.lt;
        for (auto const & node_x : scan_lines_4) {
            SDL_ASSERT(fill.Y - bbox.top() < (int)scan_lines_4.size());
            SDL_ASSERT(std::is_sorted(node_x.cbegin(), node_x.cend()));
            const size_t nodes = node_x.size();
            SDL_ASSERT(!is_odd(nodes));
            if (nodes > 1) {
                const auto * p = node_x.data();
                const auto * const last = p + nodes - 1;
                while (p < last) {
                    int const x1 = *p++;
                    int const x2 = *p++;
                    SDL_ASSERT(x1 <= x2);
                    SDL_ASSERT(x1 < grid.s_3());
                    SDL_ASSERT(x2 < grid.s_3());
                    for (fill.X = x1 + margin1; fill.X < x2; ++fill.X) {
                        if (is_break(result(make_cell_depth_4(fill, grid)))) {
                            return bc::break_;
                        }
                    }
                }
            }
            ++fill.Y;
        }
        return bc::continue_;
    }

    const size_t size_3 = 1 + (bbox.bottom() / b_3) - (bbox.top() / b_3);
    const size_t size_2 = 1 + (bbox.bottom() / b_2) - (bbox.top() / b_2);
    const size_t size_1 = 1 + (bbox.bottom() / b_1) - (bbox.top() / b_1);

    SDL_TRACE_DEBUG_2("size_4 = ", rect_height(bbox));
    SDL_TRACE_DEBUG_2("size_3 = ", size_3);
    SDL_TRACE_DEBUG_2("size_2 = ", size_2);
    SDL_TRACE_DEBUG_2("size_1 = ", size_1);

    scan_lines_int scan_lines_3(size_3);
    scan_lines_int scan_lines_2(size_2);
    scan_lines_int scan_lines_1(size_1);

    const int top_3 = bbox.top() / b_3; // scan_lines_3
    const int top_2 = bbox.top() / b_2; // scan_lines_2
    const int top_1 = bbox.top() / b_1; // scan_lines_1
    {
        int fill_Y = bbox.top();
        for (auto const & node_x : scan_lines_4) {
            SDL_ASSERT(fill_Y - bbox.top() < (int)scan_lines_4.size());
            SDL_ASSERT(std::is_sorted(node_x.cbegin(), node_x.cend()));
            const size_t nodes = node_x.size();
            SDL_ASSERT(!is_odd(nodes));
            if (nodes > 1) {
                const int y_3 = (fill_Y / b_3) - top_3;
                const int y_2 = (fill_Y / b_2) - top_2;
                const int y_1 = (fill_Y / b_1) - top_1;
                SDL_ASSERT(y_3 < size_3);
                SDL_ASSERT(y_2 < size_2);
                SDL_ASSERT(y_1 < size_1);
                auto & lines_3 = scan_lines_3[y_3];
                auto & lines_2 = scan_lines_2[y_2];
                auto & lines_1 = scan_lines_1[y_1];
                const bool empty_3 = lines_3.empty();
                const bool empty_2 = lines_2.empty();
                const bool empty_1 = lines_1.empty();
                const auto * p = node_x.data();
                const auto * const last = p + nodes - 1;
                int index = 0;
                while (p < last) {
                    int const x1 = *p++;
                    int const x2 = *p++;
                    SDL_ASSERT(x1 <= x2);
                    SDL_ASSERT(x1 < t_3);
                    SDL_ASSERT(x2 < t_3);
                    if (empty_3) {
                        lines_3.push_back(x1 / b_3);
                        lines_3.push_back(x2 / b_3);
                    }
                    else {
                        SDL_ASSERT((index + 1) < lines_3.size());
                        set_max(lines_3[index], x1 / b_3);
                        set_min(lines_3[index + 1], x2 / b_3);
                        SDL_ASSERT(lines_3[index] <= lines_3[index + 1]);
                    }
                    if (empty_2) {
                        lines_2.push_back(x1 / b_2);
                        lines_2.push_back(x2 / b_2);
                    }
                    else {
                        SDL_ASSERT((index + 1) < lines_2.size());
                        set_max(lines_2[index], x1 / b_2);
                        set_min(lines_2[index + 1], x2 / b_2);
                        SDL_ASSERT(lines_2[index] <= lines_2[index + 1]);
                    }
                    if (empty_1) {
                        lines_1.push_back(x1 / b_1);
                        lines_1.push_back(x2 / b_1);
                    }
                    else {
                        SDL_ASSERT((index + 1) < lines_1.size());
                        set_max(lines_1[index], x1 / b_1);
                        set_min(lines_1[index + 1], x2 / b_1);
                        SDL_ASSERT(lines_1[index] <= lines_1[index + 1]);
                    }
                    index += 2;
                }
            }
            ++fill_Y;
        }
    }
    {
        int fill_Y = bbox.top();
        for (auto const & node_x : scan_lines_1) {
            const size_t nodes = node_x.size();
            SDL_ASSERT(!is_odd(nodes));
            if (nodes > 1) { // 2, 4
                const int y_1 = fill_Y / b_1;
                SDL_ASSERT((y_1 - top_1) < size_1);
                const auto * p = node_x.data();
                const auto * const last = p + nodes - 1;
                while (p < last) {
                    int const x1 = *p++;
                    int const x2 = *p++;
                    SDL_ASSERT(x1 <= x2);
                    SDL_ASSERT(x1 < t_0);
                    SDL_ASSERT(x2 < t_0);
                    for (int x = x1 + margin1; x < x2; ++x) {
                        if (is_break(result(make_cell_depth_1({x, y_1}, grid)))) {
                            return bc::break_;
                        }
                    }
                }
            }
            fill_Y += b_1;
        }
    }
    {
        int fill_Y = bbox.top();
        for (auto const & node_x : scan_lines_2) {
            const size_t nodes = node_x.size();
            SDL_ASSERT(!is_odd(nodes));
            if (nodes > 1) { // 2, 4
                const int y_1 = fill_Y / b_1;
                const int y_2 = fill_Y / b_2;
                SDL_ASSERT((y_1 - top_1) < size_1);
                const auto & lines_1 = scan_lines_1[y_1 - top_1];
                if ((nodes == 2) && (lines_1.size() == 2)) {
                    int const x1 = node_x[0];
                    int const x2 = node_x[1];
                    SDL_ASSERT(x1 <= x2);
                    SDL_ASSERT(x1 < t_1);
                    SDL_ASSERT(x2 < t_1);
                    int const x11 = lines_1[0];
                    int const x22 = lines_1[1];
                    SDL_ASSERT(x11 <= x22);
                    SDL_ASSERT(x11 < t_0);
                    SDL_ASSERT(x22 < t_0);
                    if ((x11 + margin1) < x22) {
                        const int lh = (x11 + margin1) * grid.get<1>(); // scan_lines_1 => scan_lines_2
                        const int rh = x22 * grid.get<1>();
                        SDL_ASSERT(lh < rh);
                        SDL_ASSERT(lh >= x1);
                        SDL_ASSERT(rh <= x2);
                        SDL_ASSERT(rh < t_1);
                        for (int x = x1 + margin1; x < lh; ++x) {
                            if (is_break(result(make_cell_depth_2({x, y_2}, grid)))) {
                                return bc::break_;
                            }
                        }
                        for (int x = rh; x < x2; ++x) {                    
                            if (is_break(result(make_cell_depth_2({x, y_2}, grid)))) {
                                return bc::break_;
                            }
                        }
                    }
                    else {
                        for (int x = x1 + margin1; x < x2; ++x) {                    
                            if (is_break(result(make_cell_depth_2({x, y_2}, grid)))) {
                                return bc::break_;
                            }
                        }
                    }
                }
                else {
                    const auto * p = node_x.data();
                    const auto * const last = p + nodes - 1;
                    while (p < last) {
                        int const x1 = *p++;
                        int const x2 = *p++;
                        SDL_ASSERT(x1 <= x2);
                        SDL_ASSERT(x1 < t_1);
                        SDL_ASSERT(x2 < t_1);
                        for (int x = x1 + margin1; x < x2; ++x) {                    
                            if (is_break(result(make_cell_depth_2({x, y_2}, grid)))) {
                                return bc::break_;
                            }
                        }
                    }
                }
            }
            fill_Y += b_2;
        }
    }
    {
        int fill_Y = bbox.top();
        for (auto const & node_x : scan_lines_3) {
            const size_t nodes = node_x.size();
            SDL_ASSERT(!is_odd(nodes));
            if (nodes > 1) { // 2, 4
                const int y_2 = fill_Y / b_2;
                const int y_3 = fill_Y / b_3;
                SDL_ASSERT((y_2 - top_2) < size_2);
                SDL_ASSERT((y_3 - top_3) < size_3);
                const auto & lines_2 = scan_lines_2[y_2 - top_2];
                if ((nodes == 2) && (lines_2.size() == 2)) {
                    int const x1 = node_x[0];
                    int const x2 = node_x[1];
                    SDL_ASSERT(x1 <= x2);
                    SDL_ASSERT(x1 < t_2);
                    SDL_ASSERT(x2 < t_2);
                    int const x11 = lines_2[0];
                    int const x22 = lines_2[1];
                    SDL_ASSERT(x11 <= x22);
                    SDL_ASSERT(x11 < t_1);
                    SDL_ASSERT(x22 < t_1);
                    if ((x11 + margin1) < x22) {
                        const int lh = (x11 + margin1) * grid.get<2>(); // scan_lines_2 => scan_lines_3
                        const int rh = x22 * grid.get<2>();
                        SDL_ASSERT(lh < rh);
                        SDL_ASSERT(lh >= x1);
                        SDL_ASSERT(rh <= x2);
                        SDL_ASSERT(rh < t_2);
                        for (int x = x1 + margin1; x < lh; ++x) {     
                            if (is_break(result(make_cell_depth_3({ x, y_3 }, grid)))) {
                                return bc::break_;
                            }
                        }
                        for (int x = rh; x < x2; ++x) {                    
                            if (is_break(result(make_cell_depth_3({x, y_3}, grid)))) {
                                return bc::break_;
                            }
                        }
                    }
                    else {
                        for (int x = x1 + margin1; x < x2; ++x) {
                            if (is_break(result(make_cell_depth_3({x, y_3}, grid)))) {
                                return bc::break_;
                            }
                        }
                    }
                }
                else {
                    const auto * p = node_x.data();
                    const auto * const last = p + nodes - 1;
                    while (p < last) {
                        int const x1 = *p++;
                        int const x2 = *p++;
                        SDL_ASSERT(x1 <= x2);
                        SDL_ASSERT(x1 < t_2);
                        SDL_ASSERT(x2 < t_2);
                        for (int x = x1 + margin1; x < x2; ++x) {
                            if (is_break(result(make_cell_depth_3({x, y_3}, grid)))) {
                                return bc::break_;
                            }
                        }
                    }
                }
            }
            fill_Y += b_3;
        }
    }
    {
        int fill_Y = bbox.top();
        for (auto const & node_x : scan_lines_4) {
            const size_t nodes = node_x.size();
            SDL_ASSERT(!is_odd(nodes));
            if (nodes > 1) { // 2, 4
                const int y_3 = fill_Y / b_3;
                SDL_ASSERT((y_3 - top_3) < size_3);
                const auto & lines_3 = scan_lines_3[y_3 - top_3];
                if ((nodes == 2) && (lines_3.size() == 2)) {
                    int const x1 = node_x[0];
                    int const x2 = node_x[1];
                    SDL_ASSERT(x1 <= x2);
                    SDL_ASSERT(x1 < t_3);
                    SDL_ASSERT(x2 < t_3);
                    int const x11 = lines_3[0];
                    int const x22 = lines_3[1];
                    SDL_ASSERT(x11 <= x22);
                    SDL_ASSERT(x11 < t_2);
                    SDL_ASSERT(x22 < t_2);
                    if ((x11 + margin1) < x22) {
                        const int lh = (x11 + margin1) * grid.get<3>(); // scan_lines_3 => scan_lines_4
                        const int rh = x22 * grid.get<3>();
                        SDL_ASSERT(lh < rh);
                        SDL_ASSERT(lh >= x1);
                        SDL_ASSERT(rh <= x2);
                        SDL_ASSERT(rh < t_3);
                        for (int x = x1 + margin1; x < lh; ++x) {
                            if (is_break(result(make_cell_depth_4({x, fill_Y}, grid)))) {
                                return bc::break_;
                            }
                        }
                        for (int x = rh; x < x2; ++x) {                    
                            if (is_break(result(make_cell_depth_4({x, fill_Y}, grid)))) {
                                return bc::break_;
                            }
                        }
                    }
                    else {
                        for (int x = x1 + margin1; x < x2; ++x) {
                            if (is_break(result(make_cell_depth_4({x, fill_Y}, grid)))) {
                                return bc::break_;
                            }
                        }
                    }
                }
                else {
                    const auto * p = node_x.data();
                    const auto * const last = p + nodes - 1;
                    while (p < last) {
                        int const x1 = *p++;
                        int const x2 = *p++;
                        SDL_ASSERT(x1 <= x2);
                        SDL_ASSERT(x1 < t_3);
                        SDL_ASSERT(x2 < t_3);
                        for (int x = x1 + margin1; x < x2; ++x) {
                            if (is_break(result(make_cell_depth_4({x, fill_Y}, grid)))) {
                                return bc::break_;
                            }
                        }
                    }
                }
            }
            ++fill_Y;
        }
    }
    return bc::continue_;
}

#if 0
void math::fill_internal(interval_cell & result,
                         scan_lines_int const & scan_lines,
                         rect_XY const & bbox,
                         spatial_grid const grid)
{
    enum { margin1 = 1 };
    SDL_ASSERT(bbox.is_valid());
    XY fill = bbox.lt;
    for (auto const & node_x : scan_lines) {
        SDL_ASSERT(fill.Y - bbox.top() < (int)scan_lines.size());
        SDL_ASSERT(std::is_sorted(node_x.cbegin(), node_x.cend()));
        const size_t nodes = node_x.size();
        SDL_ASSERT(!is_odd(nodes));
        if (nodes > 1) {
            const auto * p = node_x.data();
            const auto * const last = p + nodes - 1;
            while (p < last) {
                int const x1 = *p++;
                int const x2 = *p++;
                SDL_ASSERT(x1 <= x2);
                SDL_ASSERT(x1 < grid.s_3());
                SDL_ASSERT(x2 < grid.s_3());
                for (fill.X = x1 + margin1; fill.X < x2; ++fill.X) {
                    result.insert(make_cell_depth_4(fill, grid));
                }
            }
        }
        ++fill.Y;
    }
}
#endif

break_or_continue math::fill_poly(function_ref result, 
                     point_2D const * const verts_2D,
                     point_2D const * const verts_2D_end,
                     spatial_grid const grid)
{
    SDL_ASSERT(verts_2D < verts_2D_end);
    rect_XY bbox;
    rasterization_::get_bbox(bbox, verts_2D, verts_2D_end, grid);
    scan_lines_int scan_lines(rect_height(bbox) + 1);
    { // plot contour
        enum { scale_id = 4 }; // experimental
        enum { max_id = spatial_grid::s_3() * scale_id }; // 65536 * 4 = 262144
        static_assert(max_id == 262144, "");
        const size_t verts_size = verts_2D_end - verts_2D;
        XY old_point { -1, -1 };
        for (size_t i = 0, j = verts_size - 1; i < verts_size; j = i++) {
            point_2D const & p1 = verts_2D[j];
            point_2D const & p2 = verts_2D[i];
            { // plot_line(p1, p2)
                int x0 = globe_to_cell_::min_max<max_id - 1>(max_id * p1.X);
                int y0 = globe_to_cell_::min_max<max_id - 1>(max_id * p1.Y);
                const int x1 = globe_to_cell_::min_max<max_id - 1>(max_id * p2.X);
                const int y1 = globe_to_cell_::min_max<max_id - 1>(max_id * p2.Y);   
                const int dx = a_abs(x1 - x0);
                const int dy = -a_abs(y1 - y0);
                const int sx = (x0 < x1) ? 1 : -1;
                const int sy = (y0 < y1) ? 1 : -1;    
                int err = dx + dy, e2;  // error value e_xy
                XY point;
                XY old_scan = { -1, -1 };
                for (;;) {
                    SDL_ASSERT(x0 >= 0);
                    SDL_ASSERT(y0 >= 0);
                    point.X = x0 / scale_id;
                    point.Y = y0 / scale_id;
                    SDL_ASSERT(point.X < grid.s_3());
                    SDL_ASSERT(point.Y < grid.s_3());
                    if ((point.X != old_point.X) || (point.Y != old_point.Y)) {
                        if (is_break(result(make_cell_depth_4(point, grid)))) {
                            return bc::break_;
                        }
                        old_point = point;
                    }
                    if (point.Y != old_scan.Y) {
                        if (old_scan.Y != -1) {
                            SDL_ASSERT(old_scan.X != -1);
                            SDL_ASSERT(a_abs(old_scan.Y - point.Y) == 1);
                            if (old_scan.Y < point.Y) { // compare with ray_crossing condition
                                scan_lines[point.Y - bbox.top()].push_sorted(point.X);
                            }
                            else { // old_scan.Y > point.Y
                                scan_lines[old_scan.Y - bbox.top()].push_sorted(old_scan.X);
                            }
                        }
                        old_scan = point;
                    }
                    e2 = 2 * err;                                   
                    if (e2 >= dy) {             // e_xy + e_x > 0
                        if (x0 == x1) break;                       
                        err += dy; x0 += sx;                       
                    }
                    if (e2 <= dx) {             // e_xy + e_y < 0
                        if (y0 == y1) break;
                        err += dx; y0 += sy;
                    }
                }        

            }
        }
    }
    return fill_internal(result, scan_lines, bbox, grid);
}

inline break_or_continue
math::fill_poly(function_ref result, buf_2D const & verts_2D, spatial_grid const grid)
{
    return fill_poly(result, verts_2D.begin(), verts_2D.end(), grid);
}

break_or_continue
math::select_range(function_ref result, spatial_point const & where, Meters const radius, spatial_grid const grid)
{
    buf_sector cross;
    buf_2D verts;
    sector_t const where_sec = spatial_sector(where);
    poly_range(cross, verts, where, radius, where_sec, grid);
    if (cross.size() < 2) {
        SDL_ASSERT(cross.empty());
        if (is_break(fill_poly(result, verts, grid))) {
            return bc::break_;
        }
    }
    else { // cross hemisphere
        SDL_ASSERT(cross.size() == 2);
        SDL_ASSERT(cross[0].sector.h != cross[1].sector.h);
        SDL_ASSERT(cross[0].index < cross[1].index);
        SDL_ASSERT(cross[1].index < verts.size());
        verts.rotate(cross[0].index + 1, cross[1].index + 1);
        size_t const middle_size = verts.size() + cross[0].index - cross[1].index;
        auto const middle = verts.begin() + middle_size;
        if (is_break(fill_poly(result, verts.begin(), middle, grid))) {
            return bc::break_;
        }
        if (is_break(fill_poly(result, middle, verts.end(), grid))) {
            return bc::break_;
        }
    }
    return bc::continue_;
}

Meters math::track_distance(spatial_point const * first,
                            spatial_point const * last,
                            spatial_point const & where,
                            spatial_rect const * const bbox)
{
    SDL_ASSERT(first < last);
    size_t const size = last - first;
    if (size < 1) {
        return 0;
    }
    if (size == 1) {
        return haversine(*first, where);
    }
    if (bbox && !bbox->is_null()) {
        SDL_ASSERT(bbox->is_valid());
        const spatial_rect & rc = *bbox;
        double min_dist = transform::infinity;
        for (--last; first < last; ++first) {
            auto const & p1 = first[0];
            auto const & p2 = first[1];
            if ((p1.longitude < rc.min_lon) && (p2.longitude < rc.min_lon))
                continue;
            if ((p1.longitude > rc.max_lon) && (p2.longitude > rc.max_lon))
                continue;
            if ((p1.latitude < rc.min_lat) && (p2.latitude < rc.min_lat))
                continue;
            if ((p1.latitude > rc.max_lat) && (p2.latitude > rc.min_lat))
                continue;
            const double dist = cross_track_distance(p1, p2, where).value();
            if (dist < min_dist) {
                min_dist = dist;
            }
        }
        //SDL_WARNING_DEBUG_2(min_dist < transform::infinity());
        return min_dist;
    }
    else {
        double min_dist = cross_track_distance(first[0], first[1], where).value();
        double dist;
        for (++first, --last; first < last; ++first) {
            dist = cross_track_distance(first[0], first[1], where).value();
            if (dist < min_dist) {
                min_dist = dist;
            }
        }
        return min_dist;
    }
}

Meters math::track_distance(spatial_point const * first,
                            spatial_point const * last,
                            spatial_point const & where)
{
    SDL_ASSERT(first < last);
    size_t const size = last - first;
    if (size < 1) {
        return 0;
    }
    if (size == 1) {
        return haversine(*first, where);
    }
    double min_dist = cross_track_distance(first[0], first[1], where).value();
    if (positive_fzero(min_dist)) {
        return 0;
    }
    double dist;
    for (++first, --last; first < last; ++first) {
        dist = cross_track_distance(first[0], first[1], where).value();
        if (dist < min_dist) {
            if (positive_fzero(dist)) {
                return 0;
            }
            min_dist = dist;
        }
    }
    SDL_ASSERT(!fzero(min_dist));
    return min_dist;
}

Meters math::min_distance(spatial_point const * first, spatial_point const * last, spatial_point const & where)
{
    SDL_ASSERT(first < last);
    size_t const size = last - first;
    if (size < 1) {
        return 0;
    }
    double min_dist = haversine(*first++, where).value();
    if (positive_fzero(min_dist)) {
        return 0;
    }
    double dist;
    for (; first < last; ++first) {
        dist = haversine(*first, where).value();
        if (dist < min_dist) {
            if (positive_fzero(dist)) {
                return 0;
            }
            min_dist = dist;
        }
    }
    SDL_ASSERT(!fzero(min_dist));
    return min_dist;
}

#if 0
//http://williams.best.vwh.net/avform.htm#Intersection
//http://williams.best.vwh.net/intersect.htm
//Intersections of two great circles
Clairauts formula:
This relates the latitude (lat) and true course (tc) along any great circle, namely:
    sin(tc)*cos(lat)=constant. 
That is, for any two points on the GC:
    sin(tc1)*cos(lat1)=sin(tc2)*cos(lat2)
Since at the highest latitude (latmx) reached the tc must be 90/270, we also have:
    latmx=acos(abs(sin(tc)*cos(lat))) 
where lat and tc are the latitude and true course at *any* point on the great circle.
#endif

#if 0 //SDL_DEBUG
Meters math::poly_distance(spatial_point const * first1, spatial_point const * const last1,
                                spatial_point const * first2, spatial_point const * const last2)
{
    SDL_ASSERT(first1 < last1);
    SDL_ASSERT(first2 < last2);
    size_t const size1 = last1 - first1;
    size_t const size2 = last2 - first2;
    if ((size1 < 1) || (size2 < 1)) {
        return 0;
    }
    if (size1 == 1) {
        return track_distance(first2, last2, *first1);
    }
    if (size2 == 1) {
        return track_distance(first1, last1, *first2);
    }
    SDL_ASSERT(size1 > 1);
    SDL_ASSERT(size2 > 1);
    SDL_ASSERT(0); // not implemented, https://en.wikipedia.org/wiki/Branch_and_bound
    return 0;
}
#endif // #if SDL_DEBUG

} // namespace space

using namespace space;

point_2D transform::project_globe(spatial_point const & p) {
    return math::project_globe(p);
}

spatial_point transform::spatial(point_2D const & p) {
    return math::reverse_project_globe(p);
}

spatial_cell transform::make_cell(spatial_point const & p, spatial_grid const g) {
    return math::globe_make_cell(p, g);
}

point_XY<int> transform::d2xy(spatial_cell::id_type const id, grid_size const size) {
    return hilbert::d2xy((int)size, id);
}

Meters transform::STDistance(spatial_point const & p1, spatial_point const & p2) {
    return math::haversine(p1, p2);
}

#if high_grid_optimization
point_2D transform::cell2point(spatial_cell const & cell, spatial_grid const grid)
{
    enum { g_0 = spatial_grid::get<0>() };
    enum { g_1 = spatial_grid::get<1>() };
    enum { g_2 = spatial_grid::get<2>() };
    enum { g_3 = spatial_grid::get<3>() };

    const XY p_0 = hilbert::n_d2xy<g_0>(cell.get_id<0>());
    const XY p_1 = hilbert::n_d2xy<g_1>(cell.get_id<1>());
    const XY p_2 = hilbert::n_d2xy<g_2>(cell.get_id<2>());
    const XY p_3 = hilbert::n_d2xy<g_3>(cell.get_id<3>());

    constexpr double f_0 = spatial_grid::f_0(); // 1.0 / g_0;
    constexpr double f_1 = spatial_grid::f_1(); // f_0 / g_1;
    constexpr double f_2 = spatial_grid::f_2(); // f_1 / g_2;
    constexpr double f_3 = spatial_grid::f_3(); // f_2 / g_3;

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
#else
point_2D transform::cell2point(spatial_cell const & cell, spatial_grid const grid)
{
    const int g_0 = grid[0];
    const int g_1 = grid[1];
    const int g_2 = grid[2];
    const int g_3 = grid[3];

    const XY p_0 = hilbert::d2xy(g_0, cell.get<0>());
    const XY p_1 = hilbert::d2xy(g_1, cell.get<1>());
    const XY p_2 = hilbert::d2xy(g_2, cell.get<2>());
    const XY p_3 = hilbert::d2xy(g_3, cell.get<3>());

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
#endif

break_or_continue
transform::cell_rect(function_cell && result, spatial_rect const & rc, spatial_grid const grid)
{
    using namespace space;
    if (!rc) {
        SDL_ASSERT(0); // not implemented
        return bc::continue_;
    }
    if (rc.cross_equator()) {
        spatial_rect r1 = rc; r1.min_lat = 0; // [0..max_lat] north
        spatial_rect r2 = rc; r2.max_lat = 0; // [min_lat..0] south
        if (is_break(math::select_hemisphere(result, r1, grid))) {
            return bc::break_;
        }
        if (is_break(math::select_hemisphere(result, r2, grid))) {
            return bc::break_;
        }
        return result.flush();
    }
    if (is_break(math::select_hemisphere(result, rc, grid))) {
        return bc::break_;
    }
    return result.flush();
}

break_or_continue
transform::cell_range(function_cell && result, spatial_point const & where, Meters const radius, spatial_grid const grid)
{
    if (fless_eq(radius.value(), 0)) {
        if (is_break(result(make_cell(where, grid)))) {
            return bc::break_;
        }
        return result.flush();
    }
    if (is_break(math::select_range(result, where, radius, grid))) {
        return bc::break_;
    }
    return result.flush();
}

#if SDL_USE_INTERVAL_CELL
namespace {
    inline void insert(interval_cell & result, spatial_cell cell) {
        switch (cell.data.depth) {
        case 4: result.insert(cell); break;
        case 3: result.insert_depth_3(cell); break;
        case 2: result.insert_depth_2(cell); break;
        case 1: result.insert_depth_1(cell); break;
        default:
            SDL_ASSERT(0);
            break;
        }
    }
}
void transform::old_cell_range(interval_cell & result, spatial_point const & where, Meters radius, spatial_grid const grid)
{
    cell_range([&result](spatial_cell cell){
        insert(result, cell);
        return bc::continue_;
    },
    where, radius, grid);
}

void transform::old_cell_rect(interval_cell & result, spatial_rect const & where, spatial_grid const grid)
{
    cell_rect([&result](spatial_cell cell){
        insert(result, cell);
        return bc::continue_;
    },
    where, grid);
}
#endif // SDL_USE_INTERVAL_CELL

bool transform::STContains(spatial_point const * first, spatial_point const * end, spatial_point const & where)
{
    return math_util::point_in_polygon(first, end, where); //FIXME: long distance on sphere (compute more intermediate points) 
}

Meters transform::STDistance(spatial_point const * first,
                             spatial_point const * end,
                             spatial_point const & where, 
                             intersect_flag const flag)
{
    switch (flag) {
    case intersect_flag::polygon:
        if (math_util::point_in_polygon(first, end, where)) {
            return 0;
        }
    case intersect_flag::linestring:
        return math::track_distance(first, end, where);
    default:
        SDL_ASSERT(flag == intersect_flag::multipoint);
        return math::min_distance(first, end, where);
    }
}

bool transform::STIntersects(spatial_rect const & rc,
                             spatial_point const * first, 
                             spatial_point const * end,
                             intersect_flag const flag)
{
    SDL_ASSERT(first < end);
    if (!rc) {
        SDL_ASSERT(0); // not implemented
        return false;
    }
    switch (flag) {
    case intersect_flag::linestring:
        return math_util::linestring_intersect(first, end, rc); //FIXME: long distance on sphere
    case intersect_flag::polygon:
        return math_util::polygon_intersect(first, end, rc); //FIXME: long distance on sphere
    default:
        SDL_ASSERT(flag == intersect_flag::multipoint);
        for (auto p = first; p < end; ++p) {
            if (rc.is_inside(*p)) {
                return true;
            }
        }
        return false;
    }
}

bool transform::STIntersects(spatial_rect const & rc, spatial_point const & where)
{
    if (!rc) {
        SDL_ASSERT(0); // not implemented
        return false;
    }
    return rc.is_inside(where); //FIXME: long distances on sphere
}

Meters transform::STLength(spatial_point const * first, spatial_point const * end)
{
    SDL_ASSERT(first < end);
    Meters length = 0;
    for (auto old = first++; first < end; ++first) {
        SDL_ASSERT((first - old) == 1);
        length += math::haversine(*old, *first);
        old = first;
    }
    return length;
}

#if SDL_DEBUG
void transform::function_cell::trace(spatial_cell const cell)
{
    if (cell.data.depth == 4)
        return; // too many cells
    static int i = 0;
    point_2D const p = transform::cell2point(cell);
    spatial_point const sp = transform::spatial(cell);
    std::cout << (i++)
        << std::setprecision(9)
        << "," << p.X
        << "," << p.Y
        << "," << sp.longitude
        << "," << sp.latitude
        << "\n";
}

void transform::function_cell::trace_call_count() const
{
    for (size_t i = 0; i < count_of(call_count); ++i) {
        SDL_TRACE("function_cell[", i, "] = ", call_count[i]);
    }
}

#endif // #if SDL_DEBUG

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
                    }
                    if (1)
                    {
                        spatial_cell x{}, y{};
                        SDL_ASSERT(!spatial_cell::less(x, y));
                        SDL_ASSERT(x == y);
                        y = spatial_cell::set_depth(y, 1);
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
                                point_XY<int> const h = transform::d2xy((uint8)d);
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
                        SDL_ASSERT(math::is_pole(spatial_point::init(Latitude(90), 0)));
                        SDL_ASSERT(math::is_pole(spatial_point::init(Latitude(-90), 0)));
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
                            double const earth_radius = limits::EARTH_RADIUS;// math::earth_radius(Latitude(0)); // depends on EARTH_ELLIPSOUD
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
                            double const earth_radius = limits::EARTH_RADIUS;//math::earth_radius(Latitude(90)); // depends on EARTH_ELLIPSOUD
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
                    if (!math::EARTH_ELLIPSOUD) {
                        const auto d1 = math::cross_track_distance(
                            SP::init(Latitude(0), 0),
                            SP::init(Latitude(0), 1),
                            SP::init(Latitude(1), -1));
                        const auto d2 = math::cross_track_distance(
                            SP::init(Latitude(0), 0),
                            SP::init(Latitude(0), 1),
                            SP::init(Latitude(1), 2));
                        const auto h1 = math::haversine(SP::init(Latitude(1), -1), SP::init(Latitude(0), 0));
                        const auto h2 = math::haversine(SP::init(Latitude(1), 2), SP::init(Latitude(0), 1));
                        SDL_ASSERT(d1.value() == h1.value());
                        SDL_ASSERT(d2.value() == h2.value());
                        SDL_ASSERT(fzero(math::cross_track_distance(
                            SP::init(Latitude(0), 0),
                            SP::init(Latitude(0), 1),
                            SP::init(Latitude(0), 0.5)).value()));
                        const auto d3 = math::cross_track_distance(
                            SP::init(Latitude(0), 0),
                            SP::init(Latitude(0), 1),
                            SP::init(Latitude(0), 2));
                        const auto h3 = math::haversine(SP::init(Latitude(0), 1), SP::init(Latitude(0), 2));
                        SDL_ASSERT(d3.value() == h3.value());
                    }
                    if (!math::EARTH_ELLIPSOUD) {
                        SDL_ASSERT_1(fequal(math::course_between_points(SP::init(Latitude(0), 0), SP::init(Latitude(0), 90)).value(), 90));
                        SDL_ASSERT_1(fequal(math::course_between_points(SP::init(Latitude(0), 0), SP::init(Latitude(0), 1)).value(), 90));
                        SDL_ASSERT_1(fequal(math::course_between_points(SP::init(Latitude(0), 0), SP::init(Latitude(0), -1)).value(), 270));
                        SDL_ASSERT_1(fequal(math::course_between_points(SP::init(Latitude(0), 0), SP::init(Latitude(1), 1)).value(), 44.995636455344851));
                        SDL_ASSERT_1(math::course_between_points(SP::init(Latitude(0), 0), SP::init(Latitude(0), 0)).value() == 0);
                    }
                    if (!math::EARTH_ELLIPSOUD) {
                        spatial_point const A = SP::init(Latitude(0), -1);
                        spatial_point const B = SP::init(Latitude(0), 1);
                        spatial_point const D1 = SP::init(Latitude(1), 0);
                        spatial_point const D2 = SP::init(Latitude(-1), 0);
                        const Meters h1 = math::haversine(D1, SP::init(0, 0));
                        const Meters d1 = math::cross_track_distance(A, B, D1);
                        const Meters d2 = math::cross_track_distance(A, B, D2);
                        SDL_ASSERT(fequal(d1.value(), d2.value()));
                        SDL_ASSERT(fless(d1.value()- h1.value(), 1e-10));
                        SDL_ASSERT(fzero(math::cross_track_distance(A, B, B).value()));
                    }
                    if (!math::EARTH_ELLIPSOUD) {
                        SDL_ASSERT(math::cross_track_distance(
                            SP::init(Latitude(90), 0),
                            SP::init(Latitude(0), 1),
                            SP::init(Latitude(0), 2)).value() == 111194.92664455874);
                        SDL_ASSERT(fzero(math::cross_track_distance(
                            SP::init(Latitude(0), 0),
                            SP::init(Latitude(0), 0),
                            SP::init(Latitude(0), 0)).value()));
                        SDL_ASSERT(math::cross_track_distance(
                            SP::init(Latitude(90), 0),
                            SP::init(Latitude(0), 1),
                            SP::init(Latitude(0), 2)).value() > 0);
                    }
                    if (1) {
                        std::vector<spatial_cell> result;
                        result.reserve(1024);
                        SDL_ASSERT(algo::unique_insertion(result, spatial_cell::init(reverse_bytes(0x00000000), 1)));
                        SDL_ASSERT(algo::unique_insertion(result, spatial_cell::init(reverse_bytes(0x01000000), 2)));
                        SDL_ASSERT(algo::unique_insertion(result, spatial_cell::init(reverse_bytes(0x01000000), 3)));
                        SDL_ASSERT(algo::unique_insertion(result, spatial_cell::init(reverse_bytes(0x01000000), 4)));
                        SDL_ASSERT(algo::unique_insertion(result, spatial_cell::init(reverse_bytes(0x01000000), 1)));
                        SDL_ASSERT(!algo::unique_insertion(result, spatial_cell::init(reverse_bytes(0x01000000), 4)));
                        SDL_ASSERT(algo::unique_insertion(result, spatial_cell::init(reverse_bytes(0x01020304), 4)));
                        SDL_ASSERT(algo::unique_insertion(result, spatial_cell::init(reverse_bytes(0x01020204), 4)));
                        SDL_ASSERT(algo::unique_insertion(result, spatial_cell::init(reverse_bytes(0x01010204), 4)));
                        SDL_ASSERT(!algo::unique_insertion(result, spatial_cell::init(reverse_bytes(0x01010204), 4)));
                        SDL_ASSERT(result.size() == 8);
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
                static void trace_cell(const spatial_cell & ) {
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
                        const double earth_radius = limits::EARTH_RADIUS;// math::earth_radius(p1, p2);
                        {
                            p2.latitude = 90.0 / 16;
                            const double h1 = math::haversine(p1, p2).value();
                            const double h2 = p2.latitude * limits::DEG_TO_RAD * earth_radius;
                            SDL_ASSERT(fequal(h1, h2));
                        }
                        {
                            p2.latitude = 90.0;
                            const double h1 = math::haversine(p1, p2).value();
                            const double h2 = p2.latitude * limits::DEG_TO_RAD * earth_radius;
                            SDL_ASSERT(fless(a_abs(h1 - h2), 1e-08));
                        }
                        /*if (math::EARTH_ELLIPSOUD) {
                            SDL_ASSERT(fequal(math::earth_radius(0), limits::EARTH_MAJOR_RADIUS));
                            SDL_ASSERT(fequal(math::earth_radius(90), limits::EARTH_MINOR_RADIUS));
                        }
                        else {
                            SDL_ASSERT(fequal(math::earth_radius(0), limits::EARTH_RADIUS));
                            SDL_ASSERT(fequal(math::earth_radius(90), limits::EARTH_RADIUS));
                        }*/
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
                            point_2D const p2 = math::project_globe(SP::init(Latitude(y), Longitude(x)));
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
                        draw_circle(SP::init(Latitude(90), Longitude(0)), Meters(100 * 1000));
                    }
                    if (print) {
                        draw_circle(SP::init(Latitude(56.3153), Longitude(44.0107)), Meters(100 * 1000));
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
