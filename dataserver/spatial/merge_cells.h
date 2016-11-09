// merge_cells.h
//
#pragma once
#ifndef __SDL_SYSTEM_MERGE_CELLS_H__
#define __SDL_SYSTEM_MERGE_CELLS_H__

#include "dataserver/spatial/spatial_type.h"

#if high_grid_optimization

namespace sdl { namespace db { namespace interval_cell_ {

template<spatial_cell::depth_t depth>
struct cell_capacity {
private:
    static_assert(depth != spatial_cell::depth_4, "");
    static constexpr spatial_cell::depth_t next_depth = (spatial_cell::depth_t)((uint8)depth + 1);
public:
    static constexpr uint32 grid = spatial_grid::grid_size::HIGH * cell_capacity<next_depth>::grid;
    static constexpr uint32 value = grid * grid;
    static constexpr uint32 upper = uint32(value - 1);
    static constexpr uint32 step = cell_capacity<next_depth>::value;
};
template<> struct cell_capacity<spatial_cell::depth_4> {
    static constexpr uint32 grid = spatial_grid::grid_size::HIGH; // = 16
    static constexpr uint32 value = grid * grid; // = 256
    static constexpr uint32 upper = uint32(value - 1);
    static constexpr uint32 step = 1;
};
template<> struct cell_capacity<spatial_cell::depth_1> {
    static constexpr uint32 grid = spatial_grid::grid_size::HIGH * cell_capacity<spatial_cell::depth_2>::grid;
    static constexpr uint64 value64 = uint64(grid) * grid; // uint64 to avoid overflow
    static constexpr uint32 upper = uint32(value64 - 1);
    static constexpr uint32 step = cell_capacity<spatial_cell::depth_2>::value;
};

template<spatial_cell::depth_t depth>
inline constexpr uint32 align_cell(const uint32 x) {
    static_assert(depth != spatial_cell::depth_1, "");
    return (x & ~uint32(cell_capacity<depth>::upper)) + cell_capacity<depth>::value;
}
template<spatial_cell::depth_t depth>
inline constexpr bool is_align_cell(const uint32 x) {
    return !(x & cell_capacity<depth>::upper);
}

template<spatial_cell::depth_t depth>
inline uint32 upper_cell(const uint32 x1, const uint32 x2) {
    static_assert((depth > 1) && (depth <= 4), "");
    SDL_ASSERT(is_align_cell<depth>(x1));
    SDL_ASSERT(x2 >= x1 + cell_capacity<depth>::upper);
    return (uint32)(x1 + ((x2 - x1 + 1) / cell_capacity<depth>::value) * cell_capacity<depth>::value - 1);
}

template<spatial_cell::depth_t depth>
inline bool merge_cells(uint32 & x11, uint32 & x22, uint32 const x1, uint32 const x2) {
    if (x2 >= x1 + cell_capacity<depth>::upper) {
        x11 = align_cell<depth>(x1);
        if (x2 >= x11 + cell_capacity<depth>::upper) {
            x22 = upper_cell<depth>(x11, x2);
            SDL_ASSERT(x1 <= x11);
            SDL_ASSERT(x11 < x22);
            SDL_ASSERT(x22 <= x2); 
            SDL_ASSERT(is_align_cell<depth>(x11));
            SDL_ASSERT(is_align_cell<depth>(x22+1));
            return true;
        }
    }
#if SDL_DEBUG
    x11 = x22 = uint32(-1);
#endif
    return false;
}

} // interval_cell_
} // db
} // sdl

#endif // high_grid_optimization
#endif // __SDL_SYSTEM_MERGE_CELLS_H__