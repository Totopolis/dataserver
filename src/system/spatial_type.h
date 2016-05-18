// spatial_type.h
//
#pragma once
#ifndef __SDL_SYSTEM_SPATIAL_TYPE_H__
#define __SDL_SYSTEM_SPATIAL_TYPE_H__

namespace sdl { namespace db {

#pragma pack(push, 1) 

struct spatial_cell { // 5 bytes

    struct data_type {
        uint8 a;
        uint8 b;
        uint8 c;
        uint8 d;
        uint8 e;
    };
    union {
        data_type data;
        uint8 id[5];
        char raw[sizeof(data_type)];
    };
};

struct spatial_point { // 16 bytes

    double Latitude;
    double Longitude;
};

#pragma pack(pop)

struct spatial_grid {
    enum grid_size {
        LOW     = 4,    // 4X4,     16 cells
        MEDIUM  = 8,    // 8x8,     64 cells
        HIGH    = 16    // 16x16,   256 cells
    };
    enum { level_size = 4 };
    grid_size level[level_size];
    spatial_grid() {
        for (auto & i : level) {
            i = grid_size::HIGH;
        }
    }
    spatial_grid(grid_size s0, grid_size s1, grid_size s2, grid_size s3) {
        level[0] = s0;
        level[1] = s1;
        level[2] = s2;
        level[3] = s3;
    }
};

struct spatial_transform : is_static {
    static spatial_point make_point(spatial_cell const &, spatial_grid const & g = spatial_grid());
    static spatial_cell make_cell(spatial_point const &, spatial_grid const & g = spatial_grid());
};

} // db
} // sdl

#endif // __SDL_SYSTEM_SPATIAL_TYPE_H__