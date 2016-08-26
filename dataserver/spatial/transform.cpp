// transform.cpp
//
#include "common/common.h"
#include "transform.h"
#include "transform_math.h"
#include "hilbert.inl"
#include <iomanip> // for std::setprecision

namespace sdl { namespace db {

#if SDL_DEBUG && defined(SDL_OS_WIN32)
namespace {
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
    void debug_trace(interval_cell const & v){
        size_t i = 0;
        v.for_each([&i](interval_cell::cell_ref cell){
            point_2D const p = transform::cell_point(cell);
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
}
#endif

using namespace space;

point_2D transform::project_globe(spatial_point const & p) {
    return math::project_globe(p);
}

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

void transform::cell_rect(interval_cell & result, spatial_rect const & rc, spatial_grid const grid)
{
    using namespace space;
    if (!rc) {
        SDL_ASSERT(0); // not implemented
        return;
    }
    if (rc.cross_equator()) {
        SDL_ASSERT((rc.min_lat < 0) && (0 < rc.max_lat));
        spatial_rect r1 = rc;
        spatial_rect r2 = rc;
        r1.min_lat = 0; // [0..max_lat] north
        r2.max_lat = 0; // [min_lat..0] south
        math::select_hemisphere(result, r1, grid);
        math::select_hemisphere(result, r2, grid);
    }
    else {
        math::select_hemisphere(result, rc, grid);
    }
#if 0
    if (!result.empty()) {
        static int trace = 0;
        if (trace++ < 1) {
            SDL_TRACE("transform::cell_rect:");
            debug_trace(result);
        }
    }
#endif
}

void transform::cell_range(interval_cell & result, spatial_point const & where, Meters const radius, spatial_grid const grid)
{
    if (fless_eq(radius.value(), 0)) {
        result.insert(make_cell(where, grid));
    }
    else {
        math::select_range(result, where, radius, grid);
#if 0
        if (!result.empty()) {
            static int trace = 0;
            if (trace++ < 1) {
                SDL_TRACE("transform::cell_range:");
                debug_trace(result);
            }
        }
#endif
    }
}

} // db
} // sdl
