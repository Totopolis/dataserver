// spatial_type.h
//
#pragma once
#ifndef __SDL_SYSTEM_SPATIAL_TYPE_H__
#define __SDL_SYSTEM_SPATIAL_TYPE_H__

namespace sdl { namespace db {

namespace unit {
    struct Latitude{};
    struct Longitude{};
}
typedef quantity<unit::Latitude, double> Latitude;
typedef quantity<unit::Longitude, double> Longitude;

struct point_double {
    double X, Y;
};

struct point_size_t {
    size_t X, Y;
};

#pragma pack(push, 1) 

struct spatial_cell { // 5 bytes
    
    static const size_t size = 4;
    using id_type = uint8;

    struct data_type { // 5 bytes
        id_type id[size];
        id_type last;
    };
    union {
        data_type data;
        char raw[sizeof(data_type)];
    };
    id_type operator[](size_t i) const {
        SDL_ASSERT(i < size);
        return data.id[i];
    }
    id_type & operator[](size_t i) {
        SDL_ASSERT(i < size);
        return data.id[i];
    }
};

inline double min_latitude()    { return -90; }
inline double max_latitude()    { return 90; }
inline double min_longitude()   { return -180; }
inline double max_longitude()   { return 180; }

struct spatial_point { // 16 bytes

    double latitude;
    double longitude;

    static bool is_valid(Latitude const d) {
        return std::fabs(d.value()) < (max_latitude() + 1e-12);
    }
    static bool is_valid(Longitude const d) {
        return std::fabs(d.value()) < (max_longitude() + 1e-12);
    }
    bool is_valid() const {
        return is_valid(Latitude(this->latitude)) && is_valid(Longitude(this->longitude));
    }
    static spatial_point init(Latitude const lat, Longitude const lon) {
        SDL_ASSERT(is_valid(lat) && is_valid(lon));
        return { lat.value(), lon.value() };
    }
};

#pragma pack(pop)

inline bool operator == (spatial_point const & x, spatial_point const & y) { 
    return (x.latitude == y.latitude) && (x.longitude == y.longitude); 
}
inline bool operator != (spatial_point const & x, spatial_point const & y) { 
    return !(x == y);
}

enum class grid_size : uint8 {
    LOW     = 4,    // 4X4,     16 cells
    MEDIUM  = 8,    // 8x8,     64 cells
    HIGH    = 16    // 16x16,   256 cells
};

struct spatial_grid {
    static const size_t size = spatial_cell::size;
    grid_size level[size];
    spatial_grid(grid_size const value = grid_size::HIGH) {
        for (auto & i : level) {
            i = value;
        }
    }
    spatial_grid(grid_size s0, grid_size s1, grid_size s2, grid_size s3) {
        level[0] = s0;
        level[1] = s1; 
        level[2] = s2;
        level[3] = s3;
        static_assert(size == 4, "");
    }
    size_t operator[](size_t i) const {
        SDL_ASSERT(i < size);
        return static_cast<size_t>(level[i]);
    }
};

struct spatial_transform : is_static {
    static spatial_point make_point(spatial_cell const &, spatial_grid const &);
    static spatial_cell make_cell(spatial_point const &, spatial_grid const &);
    static spatial_cell make_cell(Latitude, Longitude, spatial_grid const &);
private:
    static point_double map_square(spatial_point const &);
};

} // db
} // sdl

#endif // __SDL_SYSTEM_SPATIAL_TYPE_H__