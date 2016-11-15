// geography_info.h
//
#pragma once
#ifndef __SDL_SPATIAL_GEOGRAPHY_INFO_H__
#define __SDL_SPATIAL_GEOGRAPHY_INFO_H__

#include "dataserver/spatial/geography.h"

namespace sdl { namespace db {

//------------------------------------------------------------------------

struct geo_data_meta: is_static {

    typedef_col_type_n(geo_data, SRID);
    typedef_col_type_n(geo_data, tag);

    typedef TL::Seq<
        SRID
        ,tag
    >::Type type_list;
};

struct geo_data_info: is_static {
    static std::string type_meta(geo_data const &);
    static std::string type_raw(geo_data const &);
};

//------------------------------------------------------------------------

struct geo_point_meta: is_static {

    typedef_col_data_n(geo_point, data.head.SRID, SRID);
    typedef_col_data_n(geo_point, data.head.tag, tag);
    typedef_col_type_n(geo_point, point);

    typedef TL::Seq<
        SRID
        ,tag
        , point
    >::Type type_list;
};

struct geo_point_info: is_static {
    static std::string type_meta(geo_point const &);
    static std::string type_raw(geo_point const &);
};

//------------------------------------------------------------------------

struct geo_pointarray_meta: is_static {

    typedef_col_data_n(geo_pointarray, data.head.SRID, SRID);
    typedef_col_data_n(geo_pointarray, data.head.tag, tag);
    typedef_col_type_n(geo_pointarray, num_point);

    typedef TL::Seq<
        SRID
        ,tag
        ,num_point
    >::Type type_list;
};

struct geo_pointarray_info: is_static {
    static std::string type_meta(geo_pointarray const &);
    static std::string type_raw(geo_pointarray const &);
};

using geo_multipolygon_info = geo_pointarray_info;
using geo_linestring_info = geo_pointarray_info;

//------------------------------------------------------------------------

struct geo_linesegment_meta: is_static {

    typedef_col_data_n(geo_linesegment, data.head.SRID, SRID);
    typedef_col_data_n(geo_linesegment, data.head.tag, tag);
    typedef_col_data_n(geo_linesegment, data.points, points);

    typedef TL::Seq<
        SRID
        ,tag
        ,points
    >::Type type_list;
};

struct geo_linesegment_info: is_static {
    static std::string type_meta(geo_linesegment const &);
    static std::string type_raw(geo_linesegment const &);
};

//------------------------------------------------------------------------

} // db
} // sdl

#endif // __SDL_SPATIAL_GEOGRAPHY_INFO_H__