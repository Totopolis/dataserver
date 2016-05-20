// spatial_type.cpp
//
#include "common/common.h"
#include "spatial_type.h"

namespace sdl { namespace db { 

#if 0
void spatial_grid::get_step(double(&step)[size]) const
{
    double d = 1;
    for (size_t i = 0; i < size; ++i) {
        d /= double(level[i]);
        step[i] = d;
    }
}
#endif

spatial_point spatial_transform::make_point(spatial_cell const & p, spatial_grid const & g)
{
    return {};
}

point_double spatial_transform::map_square(spatial_point const & p)
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

spatial_cell spatial_transform::make_cell(spatial_point const & p, spatial_grid const & grid)
{
    enum { size = spatial_grid::size };
    const point_double d = map_square(p);

    const size_t g_0 = grid[0];
    const size_t g_1 = grid[1];
    const size_t g_2 = grid[2];
    const size_t g_3 = grid[3];

    const double d_0 = 1.0 / g_0;
    const double d_1 = d_0 / g_1;
    const double d_2 = d_1 / g_2;
    const double d_3 = d_2 / g_3;

    const point_double n_0 = { d.X / d_0, d.Y / d_0 };
    const point_double n_1 = { d.X / d_1, d.Y / d_1 };
    const point_double n_2 = { d.X / d_2, d.Y / d_2 };
    const point_double n_3 = { d.X / d_3, d.Y / d_3 };

    point_size_t p_0, p_1, p_2, p_3;

    p_0.X = a_min(static_cast<size_t>(n_0.X), g_0 - 1);
    p_0.Y = a_min(static_cast<size_t>(n_0.Y), g_0 - 1);

    p_1.X = a_min(static_cast<size_t>(n_1.X) - p_0.X * g_1, g_1 - 1);
    p_1.Y = a_min(static_cast<size_t>(n_1.Y) - p_0.Y * g_1, g_1 - 1);

    p_2.X = a_min(static_cast<size_t>(n_2.X) - p_0.X * g_1 * g_2 - p_1.X * g_2, g_2 - 1);
    p_2.Y = a_min(static_cast<size_t>(n_2.Y) - p_0.Y * g_1 * g_2 - p_1.Y * g_2, g_2 - 1);

    p_3.X = a_min(static_cast<size_t>(n_3.X) - p_0.X * g_1 * g_2 * g_3 - p_1.X * g_2 * g_3 - p_2.X * g_3, g_2 - 1);
    p_3.Y = a_min(static_cast<size_t>(n_3.Y) - p_0.Y * g_1 * g_2 * g_3 - p_1.Y * g_2 * g_3 - p_2.Y * g_3, g_2 - 1);

    const size_t id_0 = p_0.X * g_0 + p_0.Y;
    const size_t id_1 = p_1.X * g_1 + p_1.Y;
    const size_t id_2 = p_2.X * g_2 + p_2.Y;
    const size_t id_3 = p_3.X * g_3 + p_3.Y;

    spatial_cell cell {};
    cell[0] = static_cast<spatial_cell::id_type>(id_0);
    cell[1] = static_cast<spatial_cell::id_type>(id_1);
    cell[2] = static_cast<spatial_cell::id_type>(id_2);
    cell[3] = static_cast<spatial_cell::id_type>(id_3);
    return cell;
}

spatial_cell spatial_transform::make_cell(Latitude const lat, Longitude const lon, spatial_grid const & grid)
{
    return make_cell(spatial_point::init(lat, lon), grid);
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
                    A_STATIC_ASSERT_IS_POD(point_double);
                    A_STATIC_ASSERT_IS_POD(point_size_t);

                    static_assert(sizeof(spatial_cell) == 5, "");
                    static_assert(sizeof(spatial_point) == 16, "");
                    if (1) {
                        const spatial_grid low(grid_size::LOW);
                        const spatial_grid mid(grid_size::MEDIUM);
                        const spatial_grid high(grid_size::HIGH);
                        test_spatial(high);
                    }
                }
            private:
                void test_spatial(const spatial_grid & grid) {
                    spatial_cell cell;
                    if (0) {
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
                    }
                    cell = spatial_transform::make_cell(Latitude(59.53909), Longitude(150.885802), grid);
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG
