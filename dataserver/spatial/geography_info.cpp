// geography_info.cpp
//
#include "dataserver/spatial/geography_info.h"
#include "dataserver/system/page_info.h"

namespace sdl { namespace db {

//------------------------------------------------------------------------

static_col_name(geo_data_meta, SRID);
static_col_name(geo_data_meta, tag);

std::string geo_data_info::type_meta(geo_data const & row) {
    return processor_row::type_meta(row);
}

std::string geo_data_info::type_raw(geo_data const & row) {
    return to_string::type_raw(row.raw);
}

//------------------------------------------------------------------------

static_col_name(geo_point_meta, SRID);
static_col_name(geo_point_meta, tag);
static_col_name(geo_point_meta, point);

std::string geo_point_info::type_meta(geo_point const & row) {
    return processor_row::type_meta(row);
}

std::string geo_point_info::type_raw(geo_point const & row) {
    return to_string::type_raw(row.raw);
}

//------------------------------------------------------------------------

static_col_name(geo_pointarray_meta, SRID);
static_col_name(geo_pointarray_meta, tag);
static_col_name(geo_pointarray_meta, num_point);

std::string geo_pointarray_info::type_meta(geo_pointarray const & row) {
    return processor_row::type_meta(row);
}

std::string geo_pointarray_info::type_raw(geo_pointarray const & row) {
    return to_string::type_raw(row.raw);
}

//------------------------------------------------------------------------

static_col_name(geo_linesegment_meta, SRID);
static_col_name(geo_linesegment_meta, tag);
static_col_name(geo_linesegment_meta, points);

std::string geo_linesegment_info::type_meta(geo_linesegment const & row) {
    return processor_row::type_meta(row);
}

std::string geo_linesegment_info::type_raw(geo_linesegment const & row) {
    return to_string::type_raw(row.raw);
}

//------------------------------------------------------------------------

} // db
} // sdl
