// transform.cpp
//
#include "common/common.h"
#include "transform.h"
#include <cmath>

namespace sdl { namespace db { namespace {

using point_2D = point_XY<double>;
using point_3D = point_XYZ<double>;

namespace hilbert {

#define is_static_hilbert  0
#if is_static_hilbert 
static const point_XY<uint8> static_d2xy[spatial_grid::HIGH_HIGH] = {
{0,0},{1,0},{1,1},{0,1},{0,2},{0,3},{1,3},{1,2},{2,2},{2,3},{3,3},{3,2},{3,1},{2,1},{2,0},{3,0}, // 0
{4,0},{4,1},{5,1},{5,0},{6,0},{7,0},{7,1},{6,1},{6,2},{7,2},{7,3},{6,3},{5,3},{5,2},{4,2},{4,3}, // 1
{4,4},{4,5},{5,5},{5,4},{6,4},{7,4},{7,5},{6,5},{6,6},{7,6},{7,7},{6,7},{5,7},{5,6},{4,6},{4,7}, // 2
{3,7},{2,7},{2,6},{3,6},{3,5},{3,4},{2,4},{2,5},{1,5},{1,4},{0,4},{0,5},{0,6},{1,6},{1,7},{0,7}, // 3
{0,8},{0,9},{1,9},{1,8},{2,8},{3,8},{3,9},{2,9},{2,10},{3,10},{3,11},{2,11},{1,11},{1,10},{0,10},{0,11}, // 4
{0,12},{1,12},{1,13},{0,13},{0,14},{0,15},{1,15},{1,14},{2,14},{2,15},{3,15},{3,14},{3,13},{2,13},{2,12},{3,12}, // 5
{4,12},{5,12},{5,13},{4,13},{4,14},{4,15},{5,15},{5,14},{6,14},{6,15},{7,15},{7,14},{7,13},{6,13},{6,12},{7,12}, // 6
{7,11},{7,10},{6,10},{6,11},{5,11},{4,11},{4,10},{5,10},{5,9},{4,9},{4,8},{5,8},{6,8},{6,9},{7,9},{7,8}, // 7
{8,8},{8,9},{9,9},{9,8},{10,8},{11,8},{11,9},{10,9},{10,10},{11,10},{11,11},{10,11},{9,11},{9,10},{8,10},{8,11}, // 8
{8,12},{9,12},{9,13},{8,13},{8,14},{8,15},{9,15},{9,14},{10,14},{10,15},{11,15},{11,14},{11,13},{10,13},{10,12},{11,12}, // 9
{12,12},{13,12},{13,13},{12,13},{12,14},{12,15},{13,15},{13,14},{14,14},{14,15},{15,15},{15,14},{15,13},{14,13},{14,12},{15,12}, // 10
{15,11},{15,10},{14,10},{14,11},{13,11},{12,11},{12,10},{13,10},{13,9},{12,9},{12,8},{13,8},{14,8},{14,9},{15,9},{15,8}, // 11
{15,7},{14,7},{14,6},{15,6},{15,5},{15,4},{14,4},{14,5},{13,5},{13,4},{12,4},{12,5},{12,6},{13,6},{13,7},{12,7}, // 12
{11,7},{11,6},{10,6},{10,7},{9,7},{8,7},{8,6},{9,6},{9,5},{8,5},{8,4},{9,4},{10,4},{10,5},{11,5},{11,4}, // 13
{11,3},{11,2},{10,2},{10,3},{9,3},{8,3},{8,2},{9,2},{9,1},{8,1},{8,0},{9,0},{10,0},{10,1},{11,1},{11,0}, // 14
{12,0},{13,0},{13,1},{12,1},{12,2},{12,3},{13,3},{13,2},{14,2},{14,3},{15,3},{15,2},{15,1},{14,1},{14,0},{15,0}, // 15
};

static const uint8 static_xy2d[spatial_grid::HIGH][spatial_grid::HIGH] = { // [X][Y]
{0,3,4,5,58,59,60,63,64,65,78,79,80,83,84,85}, // 0
{1,2,7,6,57,56,61,62,67,66,77,76,81,82,87,86}, // 1
{14,13,8,9,54,55,50,49,68,71,72,75,94,93,88,89}, // 2
{15,12,11,10,53,52,51,48,69,70,73,74,95,92,91,90}, // 3
{16,17,30,31,32,33,46,47,122,121,118,117,96,99,100,101}, // 4
{19,18,29,28,35,34,45,44,123,120,119,116,97,98,103,102}, // 5
{20,23,24,27,36,39,40,43,124,125,114,115,110,109,104,105}, // 6
{21,22,25,26,37,38,41,42,127,126,113,112,111,108,107,106}, // 7
{234,233,230,229,218,217,214,213,128,129,142,143,144,147,148,149}, // 8
{235,232,231,228,219,216,215,212,131,130,141,140,145,146,151,150}, // 9
{236,237,226,227,220,221,210,211,132,135,136,139,158,157,152,153}, // 10
{239,238,225,224,223,222,209,208,133,134,137,138,159,156,155,154}, // 11
{240,243,244,245,202,203,204,207,186,185,182,181,160,163,164,165}, // 12
{241,242,247,246,201,200,205,206,187,184,183,180,161,162,167,166}, // 13
{254,253,248,249,198,199,194,193,188,189,178,179,174,173,168,169}, // 14
{255,252,251,250,197,196,195,192,191,190,177,176,175,172,171,170}, // 15
};
static_assert(sizeof(hilbert::static_d2xy) == 256 * 2, "");
static_assert(sizeof(hilbert::static_xy2d) == 256, "");
#endif

//https://en.wikipedia.org/wiki/Hilbert_curve
// The following code performs the mappings in both directions, 
// using iteration and bit operations rather than recursion. 
// It assumes a square divided into n by n cells, for n a power of 2,
// with integer coordinates, with (0,0) in the lower left corner, (n-1,n-1) in the upper right corner,
// and a distance d that starts at 0 in the lower left corner and goes to n^2-1 in the lower-right corner.

//rotate/flip a quadrant appropriately
inline void rot(const int n, int & x, int & y, const int rx, const int ry) {
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
    SDL_ASSERT(d < spatial_grid::HIGH_HIGH); // to be compatible with spatial_cell::id_type
    return d;
}

template<typename T>
inline T xy2d(const int n, const point_XY<int> & p) {
    A_STATIC_ASSERT_TYPE(T, spatial_cell::id_type);
    return static_cast<T>(xy2d(n, p.X, p.Y));
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

template<typename T>
inline point_XY<int> d2xy(const int n, T const d) {
    A_STATIC_ASSERT_TYPE(T, spatial_cell::id_type);
    point_XY<int> p;
    d2xy(n, d, p.X, p.Y);
    return p;
}

} // hilbert

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

inline constexpr double scalar_mul(point_3D const & p1, point_3D const & p2) {
    return p1.X * p2.X + p1.Y * p2.Y + p1.Z * p2.Z;
}
inline constexpr double scalar_mul(point_2D const & p1, point_2D const & p2) {
    return p1.X * p2.X + p1.Y * p2.Y;
}
inline constexpr point_3D multiply(point_3D const & p, double const d) {
    return { p.X * d, p.Y * d, p.Z * d };
}
#if 0 // unused
inline void trace(point_2D const & p) {
    SDL_TRACE("(", p.X, ",", p.Y, ")");
}
inline void trace(point_3D const & p) {
    SDL_TRACE("(", p.X, ",", p.Y, ",", p.Z, ")");
}
inline bool is_null(const point_3D & p) {
    return p == point_3D{};
}
inline point_3D add_point(point_3D const & p1, point_3D const & p2) {
    return { p1.X + p2.X, p1.Y + p2.Y, p1.Z + p2.Z };
}
inline int fsign(double const v) {
    return (v > 0) ? 1 : ((v < 0) ? -1 : 0);
}
#endif

inline constexpr point_3D minus_point(point_3D const & p1, point_3D const & p2) {
    return { p1.X - p2.X, p1.Y - p2.Y, p1.Z - p2.Z };
}
inline constexpr point_2D minus_point(point_2D const & p1, point_2D const & p2) {
    return { p1.X - p2.X, p1.Y - p2.Y };
}
constexpr bool point_on_plane(const point_3D & p, const point_3D & V0, const point_3D & N) {
    return fequal(scalar_mul(N, minus_point(p, V0)), 0.0);
}
inline double length(const point_3D & p) {
    return std::sqrt(scalar_mul(p, p));
}
inline double length(const point_2D & p) {
    return std::sqrt(scalar_mul(p, p));
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

#if 0
inline point_3D line_plane_intersect(spatial_point const p) { 
    return line_plane_intersect(p.latitude, p.longitude);
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
#endif

size_t longitude_quadrant(double const x) {
    SDL_ASSERT(a_abs(x) <= 180);
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

inline bool frange(double const x, double const left, double const right) {
    SDL_ASSERT(left < right);
    return fless_eq(left, x) && fless_eq(x, right);
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

inline point_XY<int> min_max(const point_2D & p, const int _max) {
    return{
        a_max<int>(a_min<int>(static_cast<int>(p.X), _max), 0),
        a_max<int>(a_min<int>(static_cast<int>(p.Y), _max), 0)
    };
};

inline point_2D fraction(const point_2D & pos_0, const point_XY<int> & h_0, const int g_0) {
    return {
        g_0 * (pos_0.X - h_0.X * 1.0 / g_0),
        g_0 * (pos_0.Y - h_0.Y * 1.0 / g_0)
    };
}

inline point_2D scale(const int scale, const point_2D & pos_0) {
    return {
        scale * pos_0.X,
        scale * pos_0.Y
    };
}

spatial_cell globe_cell(const point_2D & globe, spatial_grid const grid)
{
    const int g_0 = grid[0];
    const int g_1 = grid[1];
    const int g_2 = grid[2];
    const int g_3 = grid[3];

    SDL_ASSERT_1(space::frange(globe.X, 0, 1));
    SDL_ASSERT_1(space::frange(globe.Y, 0, 1));

    const point_XY<int> h_0 = min_max(scale(g_0, globe), g_0 - 1);
    const point_2D fraction_0 = fraction(globe, h_0, g_0);

    SDL_ASSERT_1(space::frange(fraction_0.X, 0, 1));
    SDL_ASSERT_1(space::frange(fraction_0.Y, 0, 1));

    const point_XY<int> h_1 = min_max(scale(g_1, fraction_0), g_1 - 1);    
    const point_2D fraction_1 = fraction(fraction_0, h_1, g_1);

    SDL_ASSERT_1(space::frange(fraction_1.X, 0, 1));
    SDL_ASSERT_1(space::frange(fraction_1.Y, 0, 1));

    const point_XY<int> h_2 = min_max(scale(g_2, fraction_1), g_2 - 1);    
    const point_2D fraction_2 = fraction(fraction_1, h_2, g_2);

    SDL_ASSERT_1(space::frange(fraction_2.X, 0, 1));
    SDL_ASSERT_1(space::frange(fraction_2.Y, 0, 1));

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
} // namespace

spatial_cell transform::make_cell(spatial_point const & p, spatial_grid const grid)
{
    return space::globe_cell(space::project_globe(p), grid);
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

    point_2D pos;
    pos.X = p_0.X * f_0;
    pos.Y = p_0.Y * f_0;
    pos.X += p_1.X * f_1;
    pos.Y += p_1.Y * f_1;
    pos.X += p_2.X * f_2;
    pos.Y += p_2.Y * f_2;
    pos.X += p_3.X * f_3;
    pos.Y += p_3.Y * f_3;    
    SDL_ASSERT_1(space::frange(pos.X, 0, 1));
    SDL_ASSERT_1(space::frange(pos.Y, 0, 1));
    return pos;
}

transform::vector_cell
transform::cell_range(spatial_point const & where, Meters const radius, spatial_grid const grid)
{
    spatial_cell const target = make_cell(where, grid);
    if (radius.value() == 0) {
        return { target };
    }
    A_STATIC_ASSERT_TYPE(Meters::value_type, double);
    SDL_ASSERT(radius.value() > 0);

    //FIXME: prefer WGS 84, Semi-minor axis b = 6356752.314245 m ?
    //https://en.wikipedia.org/wiki/Haversine_formula
    //

    constexpr double meter_to_degree = limits::RAD_TO_DEG * limits::TWO_PI / limits::EARTH_RADIUS;
    static_assert(fequal(limits::EARTH_RADIUS * meter_to_degree, 360.0), "meter_to_degree");    
    const double degree = radius.value() * meter_to_degree;    
    if (0) {
        spatial_point east = where; 
        east.longitude += degree;
        if (east.is_valid()) {
            point_2D const _1 = space::project_globe(where);
            point_2D const _2 = space::project_globe(east);
            const double length = space::length(space::minus_point(_1, _2));
            SDL_TRACE("length = ", length);
        }
    }
    if (0) {
        spatial_point p1 = where;
        spatial_point p2 = where;
        p1.latitude -= degree;
        p1.longitude -= degree;
        p2.latitude += degree;
        p2.longitude += degree;
        if (p1.is_valid() && p2.is_valid()) {
            spatial_cell const c1 = make_cell(p1, grid);
            spatial_cell const c2 = make_cell(p2, grid);
            SDL_ASSERT(c1 && c2);
        }
    }
    return { target };
}

//------------------------------------------------------------

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
