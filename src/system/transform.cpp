// transform.cpp
//
#include "common/common.h"
#include "transform.h"
#include "hilbert.inl"
#include "transform.inl"
#include "page_info.h"
#include <cmath>

namespace sdl { namespace db {

using SP = spatial_point;
using XY = point_XY<int>;

namespace space { 

point_3D cartesian(Latitude const lat, Longitude const lon) {
    SDL_ASSERT(spatial_point::is_valid(lat));
    SDL_ASSERT(spatial_point::is_valid(lon));
    point_3D p;
    constexpr double DEG_TO_RAD = limits::DEG_TO_RAD;
    double const L = std::cos(lat.value() * DEG_TO_RAD);
    p.X = L * std::cos(lon.value() * DEG_TO_RAD);
    p.Y = L * std::sin(lon.value() * DEG_TO_RAD);
    p.Z = std::sin(lat.value() * DEG_TO_RAD);
    return p;
}

point_3D line_plane_intersect(Latitude const lat, Longitude const lon) { //http://geomalgorithms.com/a05-_intersect-1.html

    SDL_ASSERT((lon.value() >= 0) && (lon.value() <= 90));
    SDL_ASSERT((lat.value() >= 0) && (lat.value() <= 90));

    static const point_3D P0 { 0, 0, 0 };
    static const point_3D V0 { 1, 0, 0 };
    static const point_3D e2 { 0, 1, 0 };
    static const point_3D e3 { 0, 0, 1 };
    static const point_3D N = normalize(point_3D{ 1, 1, 1 }); // plane P be given by a point V0 on it and a normal vector N

    const point_3D ray = cartesian(lat, lon); // cartesian position on globe
    const double n_u = scalar_mul(ray, N);
    
    SDL_ASSERT(n_u > 0);
    SDL_ASSERT(fequal(scalar_mul(N, N), 1.0));
    SDL_ASSERT(fequal(scalar_mul(N, V0), N.X)); // = N.X

    SDL_ASSERT(!point_on_plane(P0, V0, N));
    SDL_ASSERT(point_on_plane(e2, V0, N));
    SDL_ASSERT(point_on_plane(e3, V0, N));

    point_3D const p = multiply(ray, N.X / n_u); // distance = N * (V0 - P0) / n_u = N.X / n_u
    SDL_ASSERT((p.X >= 0) && (p.X <= 1));
    SDL_ASSERT((p.Y >= 0) && (p.Y <= 1));
    SDL_ASSERT((p.Z >= 0) && (p.Z <= 1));
    return p;
}

inline size_t longitude_quadrant(double const x) {
    SDL_ASSERT(SP::valid_longitude(x));
    if (x >= 0) {
        if (x <= 45) return 0;
        if (x <= 135) return 1;
    }
    else {
        if (x >= -45) return 0;
        if (x >= -135) return 3;
    }
    return 2; 
}
inline size_t longitude_quadrant(Longitude const x) {
    return longitude_quadrant(x.value());
}

double longitude_meridian(double const x, const size_t quadrant) { // x = longitude
    SDL_ASSERT(quadrant < 4);
    SDL_ASSERT(a_abs(x) <= 180);
    if (x >= 0) {
        switch (quadrant) {
        case 0: return x + 45;
        case 1: return x - 45;
        default:
            SDL_ASSERT(quadrant == 2);
            return x - 135;
        }
    }
    else {
        switch (quadrant) {
        case 0: return x + 45;
        case 3: return x + 135;
        default:
            SDL_ASSERT(quadrant == 2);
            return x + 180 + 45;
        }
    }
}

point_2D scale_plane_intersect(const point_3D & p3, const size_t quadrant, const bool north_hemisphere)
{
    static const point_3D e1 { 1, 0, 0 };
    static const point_3D e2 { 0, 1, 0 };
    static const point_3D e3 { 0, 0, 1 };
    static const point_3D mid{ 0.5, 0.5, 0 };
    static const point_3D px = normalize(minus_point(e2, e1));
    static const point_3D py = normalize(minus_point(e3, mid));
    static const double lx = length(minus_point(e2, e1));
    static const double ly = length(minus_point(e3, mid));
    static const point_2D scale_02 { 0.5 / lx, 0.5 / ly };
    static const point_2D scale_13 { 1 / lx, 0.25 / ly };

    const point_3D v3 = minus_point(p3, e1);
    point_2D p2 = { scalar_mul(v3, px), scalar_mul(v3, py) };

    SDL_ASSERT_1(fequal(lx, std::sqrt(2.0)));
    SDL_ASSERT_1(fequal(ly, std::sqrt(1.5)));
    SDL_ASSERT_1(frange(p2.X, 0, lx));
    SDL_ASSERT_1(frange(p2.Y, 0, ly));

    if (quadrant % 2) { // 1, 3
        p2.X *= scale_13.X;
        p2.Y *= scale_13.Y;
        SDL_ASSERT_1(frange(p2.X, 0, 1));
        SDL_ASSERT_1(frange(p2.Y, 0, 0.25));
    }
    else { // 0, 2
        p2.X *= scale_02.X;
        p2.Y *= scale_02.Y;
        SDL_ASSERT_1(frange(p2.X, 0, 0.5));
        SDL_ASSERT_1(frange(p2.Y, 0, 0.5));
    }
    point_2D ret;
    if (north_hemisphere) {
        switch (quadrant) {
        case 0:
            ret.X = 1 - p2.Y;
            ret.Y = 0.5 + p2.X;
            break;
        case 1:
            ret.X = 1 - p2.X;
            ret.Y = 1 - p2.Y;
            break;
        case 2:
            ret.X = p2.Y;
            ret.Y = 1 - p2.X;
            break;
        default:
            SDL_ASSERT(3 == quadrant);
            ret.X = p2.X;
            ret.Y = 0.5 + p2.Y;
            break;
        }
    }
    else {
        switch (quadrant) {
        case 0:
            ret.X = 1 - p2.Y;
            ret.Y = 0.5 - p2.X;
            break;
        case 1:
            ret.X = 1 - p2.X;
            ret.Y = p2.Y;
            break;
        case 2:
            ret.X = p2.Y;
            ret.Y = p2.X;
            break;
        default:
            SDL_ASSERT(3 == quadrant);
            ret.X = p2.X;
            ret.Y = 0.5 - p2.Y;
            break;
        }
    }
    SDL_ASSERT_1(frange(ret.X, 0, 1));
    SDL_ASSERT_1(frange(ret.Y, 0, 1));
    return ret;
}

point_2D project_globe(spatial_point const & s)
{
    SDL_ASSERT(s.is_valid());      
    const bool north_hemisphere = (s.latitude >= 0);
    const size_t quadrant = longitude_quadrant(s.longitude);
    const double meridian = longitude_meridian(s.longitude, quadrant);
    SDL_ASSERT((meridian >= 0) && (meridian <= 90));    
    const point_3D p3 = line_plane_intersect(north_hemisphere ? s.latitude : -s.latitude, meridian);
    return scale_plane_intersect(p3, quadrant, north_hemisphere);
}
inline point_2D project_globe(Latitude const lat, Longitude const lon) {
    return project_globe(SP::init(lat, lon));
}

spatial_cell globe_to_cell(const point_2D & globe, spatial_grid const grid)
{
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

double norm_longitude(double x) { // wrap around meridian +/-180
    if (x > 180) {
        do {
            x -= 360;
        } while (x > 180);
    }
    else if (x < -180) {
        do {
            x += 360;
        } while (x < -180);
    }
    SDL_ASSERT(SP::valid_longitude(x));
    return x;        
}

double norm_latitude(double x) { // wrap around poles +/-90
    if (x > 180) {
        do {
            x -= 360;
        } while (x > 180);
    }
    else if (x < -180) {
        do {
            x += 360;
        } while (x < -180);
    }
    SDL_ASSERT(frange(x, -180, 180));
    if (x > 90) {
        do {
            x = 180 - x;
        } while (x > 90);
    }
    else if (x < -90) {
        do {
            x = -180 - x;
        } while (x < -90);
    }
    SDL_ASSERT(SP::valid_latitude(x));
    return x;        
}

inline double add_longitude(double const lon, double const d) {
    SDL_ASSERT(SP::valid_longitude(lon));
    return norm_longitude(lon + d);
}

inline double add_latitude(double const lat, double const d) {
    SDL_ASSERT(SP::valid_latitude(lat));
    return norm_latitude(lat + d);
}

// The shape of the Earth is well approximated by an oblate spheroid with a polar radius of 6357 km and an equatorial radius of 6378 km. 
// PROVIDED a spherical approximation is satisfactory, any value in that range will do, such as R (in km) = 6378 - 21 * sin(lat)
double earth_radius(Latitude const lat, bool_constant<true>) {
    constexpr double delta = limits::EARTH_MAJOR_RADIUS - limits::EARTH_MINOR_RADIUS;
    return limits::EARTH_MAJOR_RADIUS - delta * std::sin(a_abs(lat.value() * limits::DEG_TO_RAD));
}
inline double earth_radius(Latitude, bool_constant<false>) {
    return limits::EARTH_RADIUS;
}
inline double earth_radius(Latitude const lat) {
    return earth_radius(lat, bool_constant<limits::EARTH_ELLIPSOUD>());
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
*/
double haversine(spatial_point const & _1, spatial_point const & _2, const double R)
{
    const double dlon = limits::DEG_TO_RAD * (_2.longitude - _1.longitude);
    const double dlat = limits::DEG_TO_RAD * (_2.latitude - _1.latitude);
    const double sin_lat = sin(dlat / 2);
    const double sin_lon = sin(dlon / 2);
    const double a = sin_lat * sin_lat + 
        cos(limits::DEG_TO_RAD * _1.latitude) * 
        cos(limits::DEG_TO_RAD * _2.latitude) * sin_lon * sin_lon;
    const double c = 2 * asin(a_min(1.0, sqrt(a)));
    return c * R;
}
double haversine(spatial_point const & p1, spatial_point const & p2)
{
    const double R1 = space::earth_radius(p1.latitude);
    const double R2 = space::earth_radius(p2.latitude);
    const double R = (R1 + R2) / 2;
    return haversine(p1, p2, R);
}

/*
http://www.movable-type.co.uk/scripts/latlong.html
http://williams.best.vwh.net/avform.htm#LL
Destination point given distance and bearing from start point
Given a start point, initial bearing, and distance, 
this will calculate the destination point and final bearing travelling along a (shortest distance) great circle arc.
var lat2 = Math.asin( Math.sin(lat1)*Math.cos(d/R) + Math.cos(lat1)*Math.sin(d/R)*Math.cos(brng) );
var lon2 = lon1 + Math.atan2(Math.sin(brng)*Math.sin(d/R)*Math.cos(lat1), Math.cos(d/R)-Math.sin(lat1)*Math.sin(lat2)); */
spatial_point destination(spatial_point const & p, Meters const distance, Degree const bearing)
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
    const double lon2 = lon1 + ((fzero(y) && fzero(x)) ? 0.0 : std::atan2(y, x));
    spatial_point dest;
    dest.latitude = norm_latitude(lat2 * limits::RAD_TO_DEG);
    dest.longitude = norm_longitude(lon2 * limits::RAD_TO_DEG);
    SDL_ASSERT(dest.is_valid());
    return dest;
}

point_XY<int> quadrant_grid(size_t const quad, int const grid) {
    SDL_ASSERT(quad <= 3);
    point_XY<int> size;
    if (quad % 2) {
        size.X = grid;
        size.Y = grid / 4;
    }
    else {
        size.X = grid / 2;
        size.Y = grid / 2;
    }
    return size;
}

inline point_XY<int> multiply_grid(point_XY<int> const & p, int const grid) {
    return { p.X * grid, p.Y * grid };
}

inline size_t spatial_quadrant(point_2D const & p) {
    const bool is_north = (p.Y >= 0.5);
    point_2D const pole{ 0.5, is_north ? 0.75 : 0.25 };
    point_2D const vec { p.X - pole.X, p.Y - pole.Y };
    double arg = polar(vec).arg; // in radians
    if (!is_north) {
        arg *= -1.0;
    }
    if (arg >= 0) {
        if (arg <= limits::ATAN_1_2)
            return 0; 
        if (arg <= limits::PI - limits::ATAN_1_2)
            return 1; 
    }
    else {
        if (arg >= -limits::ATAN_1_2)
            return 0; 
        if (arg >= limits::ATAN_1_2 - limits::PI)
            return 3;
    }
    return 2;
}

} // space

spatial_cell transform::make_cell(spatial_point const & p, spatial_grid const g)
{
    return space::globe_to_cell(space::project_globe(p), g);
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
transform::cell_range(spatial_point const & where, Meters const radius, spatial_grid const grid)
{
    if (fless_eq(radius.value(), 0)) {
        return { make_cell(where, grid) };
    }
    //using namespace space;
    //const bool is_north = (where.latitude >= 0);
    //const size_t quadrant = longitude_quadrant(where.longitude);
    //build polygon contour using space::destination
    //find bound box
    //select cells using pnpoly 
    return cell_bbox(where, radius, grid);
}

vector_cell
transform::cell_bbox(spatial_point const & where, Meters const radius, spatial_grid const grid)
{
    if (fless_eq(radius.value(), 0)) {
        return { make_cell(where, grid) };
    }
    const double deg = limits::RAD_TO_DEG * radius.value() / space::earth_radius(where.latitude); // latitude angle
    const double min_lat = space::add_latitude(where.latitude, -deg);
    const double max_lat = space::add_latitude(where.latitude, deg);
    const bool over_pole = max_lat != (where.latitude + deg);
    if (over_pole) {
        SDL_ASSERT(0); //FIXME: not implemented
        return {};
    }
    SDL_ASSERT(min_lat < max_lat);
    SP const lh = space::destination(where, radius, Degree(270));
    SP const rh = space::destination(where, radius, Degree(90));
    SDL_ASSERT(fequal(lh.latitude, rh.latitude));
    spatial_rect rc;
    rc.min_lat = min_lat;
    rc.max_lat = max_lat;
    rc.min_lon = lh.longitude;
    rc.max_lon = rh.longitude;
    return cell_rect(rc, grid);
}

namespace {
#if SDL_DEBUG
    template<class T>
    void trace_contour(T const & poly) {
        for (size_t i = 0; i < poly.size(); ++i) {
            std::cout << i 
                << "," << poly[i].X
                << "," << poly[i].Y
                << "\n";
        }
    }
#endif

    struct bound_boox {
        point_2D lt, rb;
    };

    template<class T>
    bound_boox get_bbox(T begin, T end) {
        SDL_ASSERT(begin != end);
        bound_boox bb;
        bb.lt = bb.rb = *(begin++);
        for (; begin != end; ++begin) {
            auto const & p = *begin;
            set_min(bb.lt.X, p.X);
            set_min(bb.lt.Y, p.Y);
            set_max(bb.rb.X, p.X);
            set_max(bb.rb.Y, p.Y);
        }
        SDL_ASSERT(!(bb.rb < bb.lt));
        return bb;
    }

} // namespace

vector_cell
transform::cell_rect(spatial_rect const & rc, spatial_grid const grid)
{
    SDL_ASSERT(rc.is_valid());
    //1) build polygon contour
    //2) find bound box
    //3) select cells using pnpoly 
    //4) take care of quadrant and special cases
    enum { EDGE_N = 16 };
    static_assert(spatial_rect::size == 4, "");	
    using contour = std::array<point_2D, EDGE_N * spatial_rect::size>;
    contour poly;
    {
        spatial_point p1 = rc[0];
        spatial_point p2;
        size_t count = 0;
        for (size_t i = 0; i < spatial_rect::size; ++i) {
            p2 = rc[(i + 1) % spatial_rect::size];
            SDL_ASSERT(p1 != p2);
            const double dx = p2.longitude - p1.longitude;
            const double dy = p2.latitude - p1.latitude;
            for (size_t i = 0 ; i < EDGE_N; ++i) {
                const double x = p1.longitude + i * dx / EDGE_N;
                const double y = p1.latitude + i * dy / EDGE_N;
                SDL_ASSERT(count < poly.size());
                poly[count++] = space::project_globe(Latitude(y), Longitude(x));
            }
            p1 = p2;
        }
        SDL_ASSERT(count == poly.size());
    }
    const bound_boox bbox = get_bbox(poly.begin(), poly.end());
    return{}; // FIXME
}

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
                        using namespace space;
                        A_STATIC_ASSERT_TYPE(point_2D::type, double);
                        A_STATIC_ASSERT_TYPE(point_3D::type, double);                        
                        SDL_ASSERT_1(cartesian(Latitude(0), Longitude(0)) == point_3D{1, 0, 0});
                        SDL_ASSERT_1(cartesian(Latitude(0), Longitude(90)) == point_3D{0, 1, 0});
                        SDL_ASSERT_1(cartesian(Latitude(90), Longitude(0)) == point_3D{0, 0, 1});
                        SDL_ASSERT_1(cartesian(Latitude(90), Longitude(90)) == point_3D{0, 0, 1});
                        SDL_ASSERT_1(cartesian(Latitude(45), Longitude(45)) == point_3D{0.5, 0.5, 0.70710678118654752440});
                        SDL_ASSERT_1(line_plane_intersect(Latitude(0), Longitude(0)) == point_3D{1, 0, 0});
                        SDL_ASSERT_1(line_plane_intersect(Latitude(0), Longitude(90)) == point_3D{0, 1, 0});
                        SDL_ASSERT_1(line_plane_intersect(Latitude(90), Longitude(0)) == point_3D{0, 0, 1});
                        SDL_ASSERT_1(line_plane_intersect(Latitude(90), Longitude(90)) == point_3D{0, 0, 1});
                        SDL_ASSERT_1(fequal(length(line_plane_intersect(Latitude(45), Longitude(45))), 0.58578643762690497));
                        SDL_ASSERT_1(longitude_quadrant(0) == 0);
                        SDL_ASSERT_1(longitude_quadrant(45) == 0);
                        SDL_ASSERT_1(longitude_quadrant(90) == 1);
                        SDL_ASSERT_1(longitude_quadrant(135) == 1);
                        SDL_ASSERT_1(longitude_quadrant(180) == 2);
                        SDL_ASSERT_1(longitude_quadrant(-45) == 0);
                        SDL_ASSERT_1(longitude_quadrant(-90) == 3);
                        SDL_ASSERT_1(longitude_quadrant(-135) == 3);
                        SDL_ASSERT_1(longitude_quadrant(-180) == 2);
                        SDL_ASSERT(fequal(limits::ATAN_1_2, std::atan2(1, 2)));
                        static_assert(fsign(0) == 0, "");
                        static_assert(fsign(1) == 1, "");
                        static_assert(fsign(-1) == -1, "");
                        static_assert(fzero(0), "");
                        static_assert(fzero(limits::fepsilon), "");
                        static_assert(!fzero(limits::fepsilon * 2), "");
                    }
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
                        SDL_ASSERT(fequal(space::norm_longitude(0), 0));
                        SDL_ASSERT(fequal(space::norm_longitude(180), 180));
                        SDL_ASSERT(fequal(space::norm_longitude(-180), -180));
                        SDL_ASSERT(fequal(space::norm_longitude(-180 - 90), 90));
                        SDL_ASSERT(fequal(space::norm_longitude(180 + 90), -90));
                        SDL_ASSERT(fequal(space::norm_longitude(180 + 90 + 360), -90));
                        SDL_ASSERT(fequal(space::norm_latitude(0), 0));
                        SDL_ASSERT(fequal(space::norm_latitude(-90), -90));
                        SDL_ASSERT(fequal(space::norm_latitude(90), 90));
                        SDL_ASSERT(fequal(space::norm_latitude(90+10), 80));
                        SDL_ASSERT(fequal(space::norm_latitude(90+10+360), 80));
                        SDL_ASSERT(fequal(space::norm_latitude(-90-10), -80));
                        SDL_ASSERT(fequal(space::norm_latitude(-90-10-360), -80));
                        SDL_ASSERT(fequal(space::norm_latitude(-90-10+360), -80));
                    }
                    if (1) {
                        SDL_ASSERT(space::spatial_quadrant(point_2D{}) == 1);
                        SDL_ASSERT(space::spatial_quadrant(point_2D{0, 0.25}) == 2);
                        SDL_ASSERT(space::spatial_quadrant(point_2D{0.5, 0.375}) == 3);
                        SDL_ASSERT(space::spatial_quadrant(point_2D{0.5, 0.5}) == 3);
                        SDL_ASSERT(space::spatial_quadrant(point_2D{1.0, 0.25}) == 0);
                        SDL_ASSERT(space::spatial_quadrant(point_2D{1.0, 0.75}) == 0);
                        SDL_ASSERT(space::spatial_quadrant(point_2D{1.0, 1.0}) == 0);
                        SDL_ASSERT(space::spatial_quadrant(point_2D{0.5, 1.0}) == 1);
                        SDL_ASSERT(space::spatial_quadrant(point_2D{0, 0.75}) == 2);
                    }
                    if (1) {
                        Meters const d1 = space::earth_radius(Latitude(0)) * limits::PI / 2;
                        Meters const d2 = d1.value() / 2;
                        SDL_ASSERT(space::destination(SP::init(Latitude(0), Longitude(0)), d1, Degree(0)).equal(Latitude(90), Longitude(0)));
                        SDL_ASSERT(space::destination(SP::init(Latitude(0), Longitude(0)), d1, Degree(360)).equal(Latitude(90), Longitude(0)));
                        SDL_ASSERT(space::destination(SP::init(Latitude(0), Longitude(0)), d2, Degree(0)).equal(Latitude(45), Longitude(0)));
                        SDL_ASSERT(space::destination(SP::init(Latitude(0), Longitude(0)), d2, Degree(90)).equal(Latitude(0), Longitude(45)));
                        SDL_ASSERT(space::destination(SP::init(Latitude(0), Longitude(0)), d2, Degree(180)).equal(Latitude(-45), Longitude(0)));
                        SDL_ASSERT(space::destination(SP::init(Latitude(0), Longitude(0)), d2, Degree(270)).equal(Latitude(0), Longitude(-45)));
                        SDL_ASSERT(space::destination(SP::init(Latitude(90), Longitude(0)), d2, Degree(0)).equal(Latitude(45), Longitude(0)));
                    }
                    if (0) {
                        draw_grid();
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
                            space::project_globe(p1);
                            space::project_globe(p2);
                            transform::make_cell(p1, spatial_grid(spatial_grid::LOW));
                            transform::make_cell(p1, spatial_grid(spatial_grid::MEDIUM));
                            transform::make_cell(p1, spatial_grid(spatial_grid::HIGH));
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
                        SDL_ASSERT(fequal(space::haversine(p1, p2), 0));
                        {
                            p2.latitude = 90.0 / 16;
                            const double h1 = space::haversine(p1, p2, limits::EARTH_RADIUS);
                            const double h2 = p2.latitude * limits::DEG_TO_RAD * limits::EARTH_RADIUS;
                            SDL_ASSERT(fequal(h1, h2));
                        }
                        {
                            p2.latitude = 90.0;
                            const double h1 = space::haversine(p1, p2, limits::EARTH_RADIUS);
                            const double h2 = p2.latitude * limits::DEG_TO_RAD * limits::EARTH_RADIUS;
                            SDL_ASSERT(fless(a_abs(h1 - h2), 1e-08));
                        }
                        if (limits::EARTH_ELLIPSOUD) {
                            SDL_ASSERT(fequal(space::earth_radius(0), limits::EARTH_MAJOR_RADIUS));
                            SDL_ASSERT(fequal(space::earth_radius(90), limits::EARTH_MINOR_RADIUS));
                        }
                        else {
                            SDL_ASSERT(fequal(space::earth_radius(0), limits::EARTH_RADIUS));
                            SDL_ASSERT(fequal(space::earth_radius(90), limits::EARTH_RADIUS));
                        }
                    }
                }
                static void test_spatial() {
                    test_spatial(spatial_grid(spatial_grid::HIGH));
                }
                static void draw_grid() {
                    if (1) {
                        std::cout << "\ndraw_grid:\n";
                        const double sx = 16 * 4;
                        const double sy = 16 * 2;
                        const double dy = (SP::max_latitude - SP::min_latitude) / sy;
                        const double dx = (SP::max_longitude - SP::min_longitude) / sx;
                        size_t i = 0;
                        for (double y = SP::min_latitude; y <= SP::max_latitude; y += dy) {
                        for (double x = SP::min_longitude; x <= SP::max_longitude; x += dx) {
                            point_2D const p = space::project_globe(Latitude(y), Longitude(x));
                            std::cout << (i++) 
                                << "," << p.X
                                << "," << p.Y
                                << "," << x
                                << "," << y
                                << "\n";
                        }}
                    }
                    draw_circle(SP::init(Latitude(45), Longitude(0)), Meters(1000 * 1000));
                    draw_circle(SP::init(Latitude(0), Longitude(0)), Meters(1000 * 1000));
                    draw_circle(SP::init(Latitude(60), Longitude(45)), Meters(1000 * 1000));
                    draw_circle(SP::init(Latitude(85), Longitude(30)), Meters(1000 * 1000));
                    draw_circle(SP::init(Latitude(-60), Longitude(30)), Meters(1000 * 500));
                }
                static void draw_circle(SP const center, Meters const distance) {
                    //std::cout << "\ndraw_circle:\n";
                    const double bx = 1;
                    size_t i = 0;
                    for (double bearing = 0; bearing < 360; bearing += bx) {
                        SP const sp = space::destination(center, distance, Degree(bearing));
                        point_2D const p = space::project_globe(sp);
                        std::cout << (i++) 
                            << "," << p.X
                            << "," << p.Y
                            << "," << sp.longitude
                            << "," << sp.latitude
                            << "\n";
                    }
                };
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG


#if 0
vector_cell
transform::cell_range(spatial_point const & where, Meters const radius, spatial_grid const grid)
{
    using namespace space;
    spatial_cell const cell_where = make_cell(where, grid);
    if (fless_eq(radius.value(), 0)) {
        return { cell_where };
    }
    using namespace space;
    const bool is_north = (where.latitude >= 0);
    const size_t quadrant = longitude_quadrant(where.longitude);
    const double rad = radius.value() / space::earth_radius(where.latitude);
    const double deg = limits::RAD_TO_DEG * rad; // latitude angle
    const double min_lat = add_longitude(where.latitude, -deg);
    const double max_lat = add_longitude(where.latitude, deg);
    SP const lh = destination(where, radius, Degree(270));
    SP const rh = destination(where, radius, Degree(90));
    SDL_ASSERT(fequal(lh.latitude, rh.latitude));
    const double min_lon = lh.longitude;
    const double max_lon = rh.longitude;
    const point_2D p1 = project_globe(Latitude(min_lat), Longitude(min_lon));
    const point_2D p2 = project_globe(Latitude(min_lat), Longitude(max_lon));
    const point_2D p3 = project_globe(Latitude(max_lat), Longitude(min_lon));
    const point_2D p4 = project_globe(Latitude(max_lat), Longitude(max_lon));
    //build contour using space::destination
    //find bound box
    //select cells using pnpoly 
    return{}; // not implemented
}
#endif