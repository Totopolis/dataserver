// transform.cpp
//
#include "common/common.h"
#include "transform.h"
#include "hilbert.inl"
#include "space.inl"
#include <cmath>

namespace sdl { namespace db { namespace space { 

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
    SDL_ASSERT(a_abs(x) <= 180);
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

double longitude_meridian(double const x, const size_t quadrant) {
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

} // space

double transform::earth_radius(Latitude const lat)
{
    constexpr double DELTA_RADIUS = limits::EARTH_MAJOR_RADIUS - limits::EARTH_MINOR_RADIUS;
    return limits::EARTH_MAJOR_RADIUS - DELTA_RADIUS * std::sin(a_abs(lat.value() * limits::DEG_TO_RAD));
}

spatial_cell transform::make_cell(spatial_point const & p, spatial_grid const g)
{
    return space::globe_to_cell(space::project_globe(p), g);
}

point_XY<int> transform::make_hil(spatial_cell::id_type const id, grid_size const size)
{
    point_XY<int> xy;
    hilbert::d2xy(size, id, xy.X, xy.Y);
    return xy;
}

point_XY<double> transform::point(spatial_cell const & cell, spatial_grid const grid)
{
    const int g_0 = grid[0];
    const int g_1 = grid[1];
    const int g_2 = grid[2];
    const int g_3 = grid[3];

    const point_XY<int> p_0 = hilbert::d2xy(g_0, cell[0]);
    const point_XY<int> p_1 = hilbert::d2xy(g_1, cell[1]);
    const point_XY<int> p_2 = hilbert::d2xy(g_2, cell[2]);
    const point_XY<int> p_3 = hilbert::d2xy(g_3, cell[3]);

    const double f_0 = 1.0 / g_0;
    const double f_1 = f_0 / g_1;
    const double f_2 = f_1 / g_2;
    const double f_3 = f_2 / g_3;

    point_XY<double> pos;
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

#if 0
-----------------------------------------------------------------------------------------------------------------
http://www.movable-type.co.uk/scripts/gis-faq-5.1.html
The shape of the Earth is well approximated by an oblate spheroid with a polar radius of 6357 km and an equatorial radius of 6378 km. 
PROVIDED a spherical approximation is satisfactory, any value in that range will do, such as
R (in km) = 6378 - 21 * sin(lat) See the WARNING below!

WARNING: This formula for R gives but a rough approximation to the radius of curvature as a function of latitude. 
The radius of curvature varies with direction and latitude; according to Snyder 
("Map Projections - A Working Manual", by John P. Snyder, U.S. Geological Survey Professional Paper 1395, 
United States Government Printing Office, Washington DC, 1987, p24), in the plane of the meridian it is given by
R' = a * (1 - e^2) / (1 - e^2 * sin^2(lat))^(3/2)
-----------------------------------------------------------------------------------------------------------------
http://www.movable-type.co.uk/scripts/gis-faq-5.1.html
https://en.wikipedia.org/wiki/Haversine_formula
http://www.movable-type.co.uk/scripts/gis-faq-5.1.html

Haversine Formula (from R.W. Sinnott, "Virtues of the Haversine", Sky and Telescope, vol. 68, no. 2, 1984, p. 159):
dlon = lon2 - lon1
dlat = lat2 - lat1
a = sin^2(dlat/2) + cos(lat1) * cos(lat2) * sin^2(dlon/2)
c = 2 * arcsin(min(1,sqrt(a)))
d = R * c
-----------------------------------------------------------------------------------------------------------------
Polar Coordinate Flat-Earth Formula
a = pi/2 - lat1
b = pi/2 - lat2
c = sqrt(a^2 + b^2 - 2 * a * b * cos(lon2 - lon1)
d = R * c
-----------------------------------------------------------------------------------------------------------------
#endif

namespace cell_range_ {

struct triangle {
    point_2D pole;
    point_2D east, west; // base line
};

struct trapezoid {
    point_2D north_west, north_east, south_west, south_east;
};

struct multi_region {
    enum { max_size = 4 };
    trapezoid region[max_size];
    uint8 size; // 0..4
    explicit operator bool() const {
        SDL_ASSERT(size <= max_size);
        return size != 0;
    }
};

bool inside(triangle const & tr, point_2D const & L)
{
    return false;
}

triangle where_triangle(spatial_point const & where)
{
    const bool north_hemisphere = (where.latitude >= 0);
    const size_t quadrant = space::longitude_quadrant(where.longitude);
    return{};
}

double scale_longitude(triangle const & tr, point_2D const & L, const double degree)
{
    SDL_ASSERT(inside(tr, L));
    return 0;
}

double scale_latitude(triangle const & tr, point_2D const & L, const double degree)
{
    SDL_ASSERT(inside(tr, L));
    return 0;
}

point_2D offset_longitude(triangle const & tr, point_2D const & L, const double lon)
{
    SDL_ASSERT(inside(tr, L));
    return{};
}

point_2D offset_latitude(triangle const & tr, point_2D const & L, const double lon)
{
    SDL_ASSERT(inside(tr, L));
    return{};
}

multi_region make_region(spatial_point const & where, const double degree)
{
    SDL_ASSERT(frange(degree, 0, 90));
    // find quadrant
    // find triangle
    //triangle const t1 = where_triangle(where);    
    //point_2D const L = space::project_globe(where);           // center point    
    //double const lon = scale_longitude(t1, L, degree);        // horizontal offset from center point
    //double const lat = scale_latitude(t1, L, degree);         // vertical offset from center point
    //point_2D west, east, north, south;                        // move from center point
    //point_2D north_west, north_eas, south_west, south_east;   // move from center point
    return{};
}

vector_cell select_cells(multi_region const & reg)
{
    SDL_ASSERT(reg);
    return{};
}

} // cell_range_

vector_cell
transform::cell_range(spatial_point const & where, Meters const radius, spatial_grid const grid)
{
    SDL_ASSERT(where.is_valid());

    return { make_cell(where, grid) }; // FIXME: not implemented

    if (radius.value() == 0) {
        return { make_cell(where, grid) };
    }
    SDL_ASSERT(radius.value() > 0);
    const double METER_TO_DEG = limits::RAD_TO_DEG * limits::TWO_PI / earth_radius(where.latitude);
    const double degree = radius.value() * METER_TO_DEG;    

    // build multi_region of trapezoids 
    // select 4-level cells (apply grid) using intersection with region/polygon
    using namespace cell_range_;
    return select_cells(make_region(where, degree));
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
                                point_XY<int> const h = transform::make_hil(d);
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
#if 1
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
#endif
                }
                static void test_spatial() {
                    test_spatial(spatial_grid(spatial_grid::HIGH));
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG
