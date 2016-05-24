// spatial_type.cpp
//
#include "common/common.h"
#include "spatial_type.h"

#if 0 //SDL_DEBUG_maketable_$$$
#include "geography/include/GeographicLib/TransverseMercatorExact.hpp" //FIXME: to be tested
#include <exception>
#endif

namespace sdl { namespace db { namespace {

point_t<double> point2square(spatial_point const & p)
{
    SDL_ASSERT(p.is_valid());

    const double longitude = (p.longitude < 0) ? (360 - p.longitude) : p.longitude;
    const double latitude = std::fabs(p.latitude);

    double X1,Y1;

    if (longitude <= 90) {
        X1 = 0;
        Y1 = longitude / 90;
    }
    else if (longitude <= 180) {
        Y1 = 1;
        X1 = (longitude - 90) / 90;
    }
    else if (longitude <= 270) {
        X1 = 1;
        Y1 = 1 - ((longitude - 180) / 90);
    }
    else {
        Y1 = 0;
        X1 = 1 - ((longitude - 270) / 90);
    }

    SDL_ASSERT(X1 <= 1);
    SDL_ASSERT(Y1 <= 1);

    const double X = X1 + (0.5 - X1) * (latitude / 90);
    const double Y = Y1 + (0.5 - Y1) * (latitude / 90);

    SDL_ASSERT(X <= 1);
    SDL_ASSERT(Y <= 1);

    return { X, Y };
}

point_t<double> project_hemisphere(spatial_point const & p)
{

    return {};
}

namespace hilbert {

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
            x = n-1 - x;
            y = n-1 - y;
        }
        //Swap x and y
        auto t  = x;
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
} // namespace

spatial_cell spatial_transform::make_cell(spatial_point const & p, spatial_grid const & grid)
{
    const int g_0 = grid[0];
    const int g_1 = grid[1];
    const int g_2 = grid[2];
    const int g_3 = grid[3];

    point_t<int> xy;
    const auto ps = point2square(p);
    xy.X = a_min<int>(static_cast<int>(ps.X * g_0), g_0-1);
    xy.Y = a_min<int>(static_cast<int>(ps.Y * g_0), g_0-1);

    const int distance = hilbert::xy2d(g_0, xy.X, xy.Y);
    spatial_cell cell {};
    cell[0] = static_cast<spatial_cell::id_type>(distance);
    return cell;
}

point_t<int> spatial_transform::make_xy(spatial_cell const & p, spatial_grid::grid_size const grid)
{
    point_t<int> xy;
    hilbert::d2xy(grid, p[0], xy.X, xy.Y);
    return xy;
}

spatial_point spatial_transform::make_point(spatial_cell const & p, spatial_grid const & grid)
{
    const int g_0 = grid[0];
    const int g_1 = grid[1];
    const int g_2 = grid[2];
    const int g_3 = grid[3];
    return {};
}

//https://en.wikipedia.org/wiki/World_Geodetic_System#WGS84
//https://en.wikipedia.org/wiki/Transverse_Mercator_projection
//https://en.wikipedia.org/wiki/Universal_Transverse_Mercator_coordinate_system

point_t<double> spatial_transform::mercator_transverse(Latitude const lat, Longitude const lon)
{
    point_t<double> xy {};
#if 0 //SDL_DEBUG_maketable_$$$
    try {
        using namespace GeographicLib;
        const double lon0 = 135;
        TransverseMercatorExact::UTM().Forward(lon0, lat.value(), lon.value(), xy.X, xy.Y);
    }
    catch (const std::exception& e) {
        SDL_TRACE("Caught exception: ", e.what());
        SDL_ASSERT(0);
        return {};
    }
#endif
    return xy;
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
                    A_STATIC_ASSERT_IS_POD(point_t<double>);

                    static_assert(sizeof(spatial_cell) == 5, "");
                    static_assert(sizeof(spatial_point) == 16, "");
        
                    static_assert(is_power_2<1>::value, "");
                    static_assert(is_power_2<spatial_grid::LOW>::value, "");
                    static_assert(is_power_2<spatial_grid::MEDIUM>::value, "");
                    static_assert(is_power_2<spatial_grid::HIGH>::value, "");
        
                    SDL_ASSERT(is_power_two(spatial_grid::LOW));
                    SDL_ASSERT(is_power_two(spatial_grid::MEDIUM));
                    SDL_ASSERT(is_power_two(spatial_grid::HIGH));

                    test_hilbert();
                    test_spatial();

                    if (0) {
                        spatial_transform::mercator_transverse(Latitude(0), Longitude(0));
                        spatial_transform::mercator_transverse(Latitude(0), Longitude(135));
                        spatial_transform::mercator_transverse(Latitude(90), Longitude(0));
                    }
                    {
                        spatial_cell x{};
                        spatial_cell y{};
                        SDL_ASSERT(!(x < y));
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
                static void test_spatial(const spatial_grid & grid) {
                    if (1) {
                        spatial_cell cell;
                        cell = spatial_transform::make_cell(Latitude(0), Longitude(0), grid);
                        cell = spatial_transform::make_cell(Latitude(0), Longitude(180), grid);
                        cell = spatial_transform::make_cell(Latitude(90), Longitude(90), grid);
                        cell = spatial_transform::make_cell(Latitude(5), Longitude(5), grid);
                        cell = spatial_transform::make_cell(Latitude(0), Longitude(45), grid);
                        cell = spatial_transform::make_cell(Latitude(45), Longitude(0), grid);
                        cell = spatial_transform::make_cell(Latitude(0), Longitude(90), grid);
                        cell = spatial_transform::make_cell(Latitude(90), Longitude(0), grid);
                        cell = spatial_transform::make_cell(Latitude(45), Longitude(45), grid);
                        cell = spatial_transform::make_cell(Latitude(45), Longitude(180), grid);
                        cell = spatial_transform::make_cell(Latitude(90), Longitude(180), grid);
                        cell = spatial_transform::make_cell(Latitude(59.53909), Longitude(150.885802), grid);
                    }
                }
                static void test_spatial() {
                    test_spatial(spatial_grid::LOW);
                    test_spatial(spatial_grid::MEDIUM);
                    test_spatial(spatial_grid::HIGH);
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG
