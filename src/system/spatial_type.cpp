// spatial_type.cpp
//
#include "common/common.h"
#include "spatial_type.h"
#include "page_info.h"
#include <cmath>

namespace sdl { namespace db { namespace {

using point_2D = point_XY<double>;
using point_3D = point_XYZ<double>;

namespace hilbert { //FIXME: make static array to map (X,Y) <=> distance

//https://en.wikipedia.org/wiki/Hilbert_curve
// The following code performs the mappings in both directions, 
// using iteration and bit operations rather than recursion. 
// It assumes a square divided into n by n cells, for n a power of 2,
// with integer coordinates, with (0,0) in the lower left corner, (n-1,n-1) in the upper right corner,
// and a distance d that starts at 0 in the lower left corner and goes to n^2-1 in the lower-right corner.

//rotate/flip a quadrant appropriately
void rot(const int n, int & x, int & y, const int rx, const int ry) {
    SDL_ASSERT(is_power_two(n));
    if (ry == 0) {
        if (rx == 1) {
            x = n - 1 - x;
            y = n - 1 - y;
        }
        //Swap x and y
        const auto t  = x;
        x = y;
        y = t;
    }
}

//convert (x,y) to d
int xy2d(const int n, int x, int y) {
    SDL_ASSERT(is_power_two(n));
    SDL_ASSERT(x < n);
    SDL_ASSERT(y < n);
    int rx, ry, d = 0;
    for (int s = n/2; s > 0; s /= 2) {
        rx = (x & s) > 0;
        ry = (y & s) > 0;
        d += s * s * ((3 * rx) ^ ry);
        rot(s, x, y, rx, ry);
    }
    SDL_ASSERT((d >= 0) && (d < (n * n)));
    SDL_ASSERT(d < 256); // to be compatible with spatial_cell::id_type
    return d;
}

//convert d to (x,y)
void d2xy(const int n, const int d, int & x, int & y) {
    SDL_ASSERT(is_power_two(n));
    SDL_ASSERT((d >= 0) && (d < (n * n)));
    int rx, ry, t = d;
    x = y = 0;
    for (int s = 1; s < n; s *= 2) {
        rx = 1 & (t / 2);
        ry = 1 & (t ^ rx);
        rot(s, x, y, rx, ry);
        x += s * rx;
        y += s * ry;
        t /= 4;
    }
    SDL_ASSERT((x >= 0) && (x < n));
    SDL_ASSERT((y >= 0) && (y < n));
}

} // hilbert

namespace space { 

static const double PI = 3.14159265358979323846;
static const double RAD_TO_DEG = 57.295779513082321;
static const double DEG_TO_RAD = 0.017453292519943296;
static const double SQRT_2 = 1.41421356237309504880; // = sqrt(2)
static const double DIV_SQRT_2 = 1.0 / SQRT_2; // = 1/sqrt(2) or reciprocal = 0.70710678118654752440

#if SDL_DEBUG
void trace(point_2D const & p) {
    SDL_TRACE("(", p.X, ",", p.Y, ")");
}
void trace(point_3D const & p) {
    SDL_TRACE("(", p.X, ",", p.Y, ",", p.Z, ")");
}
#endif

point_3D cartesian(Latitude const lat, Longitude const lon) {
    SDL_ASSERT(spatial_point::is_valid(lat));
    SDL_ASSERT(spatial_point::is_valid(lon));
    point_3D p;
    double const L = std::cos(lat.value() * DEG_TO_RAD);
    p.X = L * std::cos(lon.value() * DEG_TO_RAD);
    p.Y = L * std::sin(lon.value() * DEG_TO_RAD);
    p.Z = std::sin(lat.value() * DEG_TO_RAD);
    return p;
}

inline double scalar_mul(point_3D const & p1, point_3D const & p2) {
    return p1.X * p2.X + p1.Y * p2.Y + p1.Z * p2.Z;
}

inline point_3D multiply(point_3D const & p, double const d) {
    return { p.X * d, p.Y * d, p.Z * d };
}

inline point_3D add_point(point_3D const & p1, point_3D const & p2) {
    return { p1.X + p2.X, p1.Y + p2.Y, p1.Z + p2.Z };
}

inline point_3D minus_point(point_3D const & p1, point_3D const & p2) {
    return { p1.X - p2.X, p1.Y - p2.Y, p1.Z - p2.Z };
}

bool point_on_plane(const point_3D & p, const point_3D & V0, const point_3D & N) {
    return fequal(scalar_mul(N, minus_point(p, V0)), 0.0);
}

inline double length(const point_3D & p) {
    return std::sqrt(scalar_mul(p, p));
}

inline bool is_null(const point_3D & p) {
    return p == point_3D{};
}

inline point_3D normalize(const point_3D & p) {
    const double d = length(p);
    SDL_ASSERT(d > 0);
    return multiply(p, 1.0 / d);
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

inline point_3D line_plane_intersect(spatial_point const p) { 
    return line_plane_intersect(p.latitude, p.longitude);
}

inline int fsign(double const v) {
    return (v > 0) ? 1 : ((v < 0) ? -1 : 0);
}

inline bool fnegative(double const v) {
    return fsign(v) < 0;
}

inline double longitude_distance(double const left, double const right) {
    SDL_ASSERT(std::fabs(left) <= 180);
    SDL_ASSERT(std::fabs(right) <= 180);
    if ((left >= 0) && (right < 0)) {
		return right - left + 360;
    }
    return right - left;
}

size_t longitude_quadrant(double const x) { //FIXME: to be tested
    SDL_ASSERT(std::fabs(x) <= 180);
    if (x >= 0) {
        if (x <= 45) return 0;
        if (x <= 135) return 1;
        return 2;
    }
    if (x >= -45) return 0;
    if (x >= -135) return 3;
    return 2; 
}

double longitude_meridian(double const x, const size_t quadrant) {
    SDL_ASSERT(quadrant < 4);
    SDL_ASSERT(std::fabs(x) <= 180);
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

inline bool frange(double const x, double const left, double const right) {
    SDL_ASSERT(left < right);
    return fless_equal(left, x) && fless_equal(x, right);
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
    const point_2D p2 = scale_plane_intersect(p3, quadrant, north_hemisphere);
    return p2;
}

} // space
} // namespace

point_XY<int> spatial_transform::make_XY(spatial_cell const & p, spatial_grid::grid_size const grid)
{
    point_XY<int> xy;
    hilbert::d2xy(grid, p[0], xy.X, xy.Y);
    return xy;
}

vector_cell spatial_transform::make_cell(spatial_point const & p, spatial_grid const & grid)
{
    const int g_0 = grid[0];

    const point_2D globe = space::project_globe(p);
    const point_2D d = {
        globe.X * g_0,
        globe.Y * g_0
    };
    const int X = a_min<int>(static_cast<int>(d.X), g_0 - 1);
    const int Y = a_min<int>(static_cast<int>(d.Y), g_0 - 1);    
    
    const int dist_0 = hilbert::xy2d(g_0, X, Y);    

    vector_cell vc(1);
    vc[0][0] = static_cast<spatial_cell::id_type>(dist_0);
    vc[0].data.last = spatial_cell::last_4; //FIXME: to be tested
    return vc;
}

spatial_point spatial_transform::make_point(spatial_cell const & p, spatial_grid const & grid)
{
    const int g_0 = grid[0];
    const int g_1 = grid[1];
    const int g_2 = grid[2];
    const int g_3 = grid[3];
    return {};
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
                    A_STATIC_ASSERT_IS_POD(spatial_cell);
                    A_STATIC_ASSERT_IS_POD(spatial_point);
                    A_STATIC_ASSERT_IS_POD(point_2D);
                    A_STATIC_ASSERT_IS_POD(point_3D);

                    static_assert(sizeof(spatial_cell) == 5, "");
                    static_assert(sizeof(spatial_point) == 16, "");
        
                    static_assert(is_power_2<1>::value, "");
                    static_assert(is_power_2<spatial_grid::LOW>::value, "");
                    static_assert(is_power_2<spatial_grid::MEDIUM>::value, "");
                    static_assert(is_power_2<spatial_grid::HIGH>::value, "");
        
                    SDL_ASSERT(is_power_two(spatial_grid::LOW));
                    SDL_ASSERT(is_power_two(spatial_grid::MEDIUM));
                    SDL_ASSERT(is_power_two(spatial_grid::HIGH));
                    {
                        spatial_cell x{};
                        spatial_cell y{};
                        SDL_ASSERT(!(x < y));
                    }
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
                    }
                }
                static void test_hilbert() {
                    spatial_grid::grid_size const sz = spatial_grid::HIGH;
                    for (int i = 0; (1 << i) <= sz; ++i) {
                        test_hilbert(1 << i);
                    }
                }
                static void trace_cell(const vector_cell & vc) {
                    for (auto & v : vc) {
                        SDL_TRACE(to_string::type(v));
                    }
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
                            spatial_transform::make_cell(p1, spatial_grid(spatial_grid::LOW));
                            spatial_transform::make_cell(p1, spatial_grid(spatial_grid::MEDIUM));
                            spatial_transform::make_cell(p1, spatial_grid(spatial_grid::HIGH));
                        }}
                    }
                    if (1) {
                        static const spatial_point test[] = { // latitude, longitude
                            { 0, 0 },
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
                            { 0, 166 },
                            { 48.7139, 44.4984 },   // cell_id = 156-163-67-177-4
                            { 55.7975, 49.2194 },   // cell_id = 157-178-149-55-4
                            { 47.2629, 39.7111 },   // cell_id = 163-78-72-221-4
                            { 47.261, 39.7068 },    // cell_id = 163-78-72-223-4
                            { 55.7831, 37.3567 },   // cell_id = 156-38-25-118-4
                            { 0, -86 },             // cell_id = 128-234-255-15-4
                            { 45, -135 },           // cell_id = 70-170-170-170-4
                            { 45, 135 },            // cell_id = 91-255-255-255-4
                            { 45, 0 },              // cell_id = 160-236-255-239-4 | 181-153-170-154-4
                            { 45, -45 },            // cell_id = 134-170-170-170-4 | 137-255-255-255-4 | 182-0-0-0-4 | 185-85-85-85-4
                        };
                        for (size_t i = 0; i < A_ARRAY_SIZE(test); ++i) {
                            std::cout << i << ": " << to_string::type(test[i]) << " => ";
                            trace_cell(spatial_transform::make_cell(test[i], grid));
                        }
                        SDL_TRACE();
                    }
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

#if 0 // not optimized
double longitude_distance(double const left, double const right)
{
    SDL_ASSERT(std::fabs(left) <= 180);
    SDL_ASSERT(std::fabs(right) <= 180);    
    if (left >= 0) {
        if (right >= 0) {
            SDL_ASSERT(right >= left);
            return right - left;
        }
        return (right + 180) + (180 - left);
    }
    else {
        if (right < 0) {
            SDL_ASSERT(right >= left);
            return right - left;
        }
        return right - left;
    }
}
#endif