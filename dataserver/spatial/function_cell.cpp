// function_cell.cpp
//
#include "dataserver/spatial/function_cell.h"
#include "dataserver/spatial/transform.h"
#include <iomanip> // for std::setprecision

namespace sdl { namespace db {

#if SDL_DEBUG
void base_function_cell<true>::trace(spatial_cell const cell)
{
    if (0) {
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
}

void base_function_cell<true>::trace_call_count() const
{
    for (size_t i = 0; i < count_of(call_count); ++i) {
        SDL_TRACE("function_cell[", i, "] = ", call_count[i]);
    }
}

#endif // #if SDL_DEBUG

} // db
} // sdl
