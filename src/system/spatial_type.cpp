// spatial_type.cpp
//
#include "common/common.h"
#include "spatial_type.h"
#include "page_info.h"

namespace sdl { namespace db { namespace {

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

point_t<double> project_globe(spatial_point const & p)
{
    SDL_ASSERT(p.is_valid());  
    point_t<double> p1, p2, p3;
    double meridian, sector;
    p3.X = 0.5;
    if (p.latitude < 0) { // south hemisphere
        p3.Y = 0.25;
        if (p.longitude >= 0) { // north-east            
            if (p.longitude <= 45) {
                p1 = { 1, 0.25 };
                p2 = { 1, 0 };
                meridian = 0;
                sector = 45;
            }
            else if (p.longitude <= 135) {
                p1 = { 1, 0 };
                p2 = { 0, 0 };
                meridian = 45;
                sector = 90;
            }
            else {
                SDL_ASSERT(p.longitude <= 180);
                p1 = { 0, 0 };
                p2 = { 0, 0.25 };
                meridian = 135;
                sector = 45;
            }
        }
        else { // north-west
            if (p.longitude >= -45) {
                p1 = { 1, 0.5 };
                p2 = { 1, 0.25 };
                meridian = -45;
                sector = 45;
            }
            else if (p.longitude >= -135) {
                p1 = { 0, 0.5 };
                p2 = { 1, 0.5 };
                meridian = -135;
                sector = 90;
            }
            else {
                SDL_ASSERT(-180 <= p.longitude);
                p1 = { 0, 0.25 };
                p2 = { 0, 0.5 };
                meridian = -180;
                sector = 45;
            }
        }
    }
    else { // north hemisphere
        p3.Y = 0.75;
        if (p.longitude >= 0) { // south-east           
            if (p.longitude <= 45) {
                p1 = { 1, 0.75 };
                p2 = { 1, 1 };
                meridian = 0;
                sector = 45;
            }
            else if (p.longitude <= 135) {
                p1 = { 1, 1 };
                p2 = { 0, 1 };
                meridian = 45;
                sector = 90;
            }
            else {
                SDL_ASSERT(p.longitude <= 180);
                p1 = { 0, 1 };
                p2 = { 0, 0.75 };
                meridian = 135;
                sector = 45;
            }
        }
        else { // south-west
            if (p.longitude >= -45) {
                p1 = { 1, 0.5 };
                p2 = { 1, 0.75 };
                meridian = -45;
                sector = 45;
            }
            else if (p.longitude >= -135) {
                p1 = { 0, 0.5 };
                p2 = { 1, 0.5 };
                meridian = -135;
                sector = 90;
            }
            else {
                SDL_ASSERT(-180 <= p.longitude);
                p1 = { 0, 0.75 };
                p2 = { 0, 0.5 };
                meridian = -180;
                sector = 45;
            }
        }
    }
    SDL_ASSERT(p.longitude >= meridian);
    SDL_ASSERT((p.longitude - meridian) <= sector);
    double const move_longitude = (p.longitude - meridian) / sector;
    SDL_ASSERT(move_longitude <= 1);
    const point_t<double> base = {
        p1.X + (p2.X - p1.X) * move_longitude, 
        p1.Y + (p2.Y - p1.Y) * move_longitude
    };
    double const move_latitude = std::fabs(p.latitude) / 90.0;
    const point_t<double> result = {
        base.X + (p3.X - base.X) * move_latitude,
        base.Y + (p3.Y - base.Y) * move_latitude
    };
    SDL_ASSERT((result.X >= 0) && (result.Y >= 0));
    SDL_ASSERT((result.X <= 1) && (result.Y <= 1));
    return result;
}

} // namespace

point_t<int> spatial_transform::make_XY(spatial_cell const & p, spatial_grid::grid_size const grid)
{
    point_t<int> xy;
    hilbert::d2xy(grid, p[0], xy.X, xy.Y);
    return xy;
}

vector_cell spatial_transform::make_cell(spatial_point const & p, spatial_grid const & grid)
{
    const point_t<double> globe = project_globe(p);

    const int g_0 = grid[0];
    const int X = a_min<int>(static_cast<int>(globe.X * g_0), g_0 - 1);
    const int Y = a_min<int>(static_cast<int>(globe.Y * g_0), g_0 - 1);
    
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

                    {
                        spatial_cell x{};
                        spatial_cell y{};
                        SDL_ASSERT(!(x < y));
                    }
                    test_hilbert();
                    test_spatial();
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
                    SDL_ASSERT(!vc.empty());
                    for (auto & v : vc) {
                        SDL_TRACE(to_string::type(v));
                    }
                }
                static void test_spatial(const spatial_grid & grid) {
                    if (0) {
                        spatial_point p1{}, p2{};
                        for (int i = 0; i <= 4; ++i) {
                        for (int j = 0; j <= 2; ++j) {
                            p1.longitude = 45 * i; 
                            p2.longitude = -45 * i;
                            p1.latitude = 45 * j;
                            p2.latitude = -45 * j;
                            project_globe(p1);
                            project_globe(p2);
                            spatial_transform::make_cell(p1, spatial_grid(spatial_grid::HIGH));
                        }}
                    }
                    if (0) {
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
                            { 0, -86 },             // cell_id = 128-234-255-15-4
                            { 55.7975, 49.2194 },   // cell_id = 157-178-149-55-4
                            { 47.2629, 39.7111 },   // cell_id = 163-78-72-221-4
                            { 47.261, 39.7068 },    // cell_id = 163-78-72-223-4
                            { 55.7831, 37.3567 },   // cell_id = 156-38-25-118-4
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
