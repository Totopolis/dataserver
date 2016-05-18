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

//FIXME: make spatial_cell from geography WKT POINT (X = Longitude, Y = Latitude)

} // db
} // sdl

#endif // __SDL_SYSTEM_SPATIAL_TYPE_H__