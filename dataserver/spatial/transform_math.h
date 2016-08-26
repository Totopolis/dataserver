// transform_math.h
//
#pragma once
#ifndef __SDL_SYSTEM_TRANSFORM_MATH_H__
#define __SDL_SYSTEM_TRANSFORM_MATH_H__

#include "spatial_type.h"
#include "interval_cell.h"
#include "common/array.h"

namespace sdl { namespace db { namespace space { 

#define use_fill_poly_without_plot_line     0

struct math : is_static {
    enum { EARTH_ELLIPSOUD = 0 }; // to be tested
    enum quadrant {
        q_0 = 0, // [-45..45) longitude
        q_1 = 1, // [45..135)
        q_2 = 2, // [135..180][-180..-135)
        q_3 = 3  // [-135..-45)
    };
    enum { quadrant_size = 4 };
    enum class hemisphere {
        north = 0, // [0..90] latitude
        south = 1  // [-90..0)
    };
    struct sector_t {
        hemisphere h;
        quadrant q;
    };
    struct sector_index {
        sector_t sector;
        size_t index;
    };
    using sector_indexes = vector_buf<sector_index, 16>;
    using buf_XY = vector_buf<XY, 32>;
    using buf_2D = vector_buf<point_2D, 32>;
    static hemisphere latitude_hemisphere(double const lat) {
        return (lat >= 0) ? hemisphere::north : hemisphere::south;
    }
    static hemisphere latitude_hemisphere(spatial_point const & s) {
        return latitude_hemisphere(s.latitude);
    }
    static hemisphere point_hemisphere(point_2D const & p) {
        return (p.Y >= 0.5) ? hemisphere::north : hemisphere::south;
    }
    static bool latitude_pole(double const lat) {
        return fequal(lat, (lat > 0) ? 90 : -90);
    }
    static sector_t spatial_sector(spatial_point const &);
    static quadrant longitude_quadrant(double);
    static quadrant longitude_quadrant(Longitude);
    static bool cross_longitude(double mid, double left, double right);
    static double longitude_distance(double left, double right);
    static double longitude_distance(double left, double right, bool);
    static bool rect_cross_quadrant(spatial_rect const &);
    static double longitude_meridian(double, quadrant);
    static double reverse_longitude_meridian(double, quadrant);
    static point_3D cartesian(Latitude, Longitude);
    static spatial_point reverse_cartesian(point_3D const &);
    static point_3D line_plane_intersect(Latitude, Longitude);
    static point_2D scale_plane_intersect(const point_3D &, quadrant, hemisphere);
    static point_2D project_globe(spatial_point const &);
    static point_2D project_globe(spatial_point const &, hemisphere);
    static spatial_cell globe_to_cell(const point_2D &, spatial_grid);
    static spatial_cell globe_make_cell(spatial_point const & p, spatial_grid const g) {
        return globe_to_cell(project_globe(p), g);
    }
    static double norm_longitude(double);
    static double norm_latitude(double);
    static double add_longitude(double const lon, double const d);
    static double add_latitude(double const lat, double const d);
    static Meters haversine(spatial_point const &, spatial_point const &, Meters);
    static Meters haversine(spatial_point const &, spatial_point const &);
    static Meters haversine_error(spatial_point const &, spatial_point const &, Meters);
    static spatial_point destination(spatial_point const &, Meters const distance, Degree const bearing);
    static point_XY<int> quadrant_grid(quadrant, int const grid);
    static quadrant point_quadrant(point_2D const & p);
    static spatial_point reverse_line_plane_intersect(point_3D const &);
    static point_3D reverse_scale_plane_intersect(point_2D const &, quadrant, hemisphere);
    static spatial_point reverse_project_globe(point_2D const &);
    static bool destination_rect(spatial_rect &, spatial_point const &, Meters);
    static void poly_latitude(buf_2D &, double const lat, double const lon1, double const lon2, hemisphere, bool);
    static void poly_longitude(buf_2D &, double const lon, double const lat1, double const lat2, hemisphere, bool);
    static void poly_rect(buf_2D & verts, spatial_rect const &, hemisphere);
    static void poly_range(sector_indexes &, buf_2D & verts, spatial_point const &, Meters, sector_t const &, spatial_grid);
    static void fill_poly(interval_cell &, buf_2D const &, spatial_grid);
    static void fill_poly_without_plot_line(interval_cell &, buf_2D const &, spatial_grid);
    static void fill_internal_area(interval_cell &, buf_XY &, spatial_grid);
    static spatial_cell make_cell(XY const &, spatial_grid);
    static void select_hemisphere(interval_cell &, spatial_rect const &, spatial_grid);
    static void select_sector(interval_cell &, spatial_rect const &, spatial_grid);    
    static void select_range(interval_cell &, spatial_point const &, Meters, spatial_grid);
    static XY rasterization(point_2D const & p, int);
    static void rasterization(rect_XY &, rect_2D const &, spatial_grid);
    static void rasterization(buf_XY &, buf_2D const &, spatial_grid);
private:
    using ellipsoid_true = bool_constant<true>;
    using ellipsoid_false = bool_constant<false>;
    static double earth_radius(double, ellipsoid_true);
    static double earth_radius(double, double, ellipsoid_true);
    static double earth_radius(double, ellipsoid_false) {
        return limits::EARTH_RADIUS;
    }
    static double earth_radius(double, double, ellipsoid_false) {
        return limits::EARTH_RADIUS;
    }
public:
    static double earth_radius(Latitude const lat) {
        return earth_radius(lat.value(), bool_constant<EARTH_ELLIPSOUD>());
    }
    static double earth_radius(Latitude const lat1, Latitude const lat2) {
        return earth_radius(lat1.value(), lat2.value(), bool_constant<EARTH_ELLIPSOUD>());
    }
    static double earth_radius(spatial_point const & p) {
        return earth_radius(p.latitude);
    }
    static size_t remain(size_t const x, size_t const y) {
        size_t const d = x % y;
        return d ? (y - d) : 0;
    }
    static size_t roundup(double const x, size_t const y) {
        SDL_ASSERT(x >= 0);
        size_t const d = a_max(static_cast<size_t>(x + 0.5), y); 
        return d + remain(d, y); 
    }
    static const double sorted_quadrant[quadrant_size]; // longitudes
    static const point_2D north_quadrant[quadrant_size];
    static const point_2D south_quadrant[quadrant_size];
    static const point_2D pole_hemisphere[2];
    static const point_2D & sector_point(sector_t);
private:
    class rasterizer : noncopyable {
        using data_type = buf_XY;
        const int max_id;
        data_type & result;
    public:
        rasterizer(data_type & p, spatial_grid const & g)
            : max_id(g.s_3()), result(p) {}
        bool push_back(point_2D const & p) {
            XY val = rasterization(p, max_id);
            if (result.empty() || (val != result.back())) {
                result.push_back(val);
                return true;
            }
            return false;
        };
    };
};

} // space
} // db
} // sdl

#include "transform_math.inl"

#endif // __SDL_SYSTEM_TRANSFORM_MATH_H__