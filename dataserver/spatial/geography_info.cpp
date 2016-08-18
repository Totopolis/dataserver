// geography_info.cpp
//
#include "common/common.h"
#include "geography_info.h"
#include "system/page_info.h"

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
#if 0
static_col_name(geo_multipolygon_meta, SRID);
static_col_name(geo_multipolygon_meta, tag);
static_col_name(geo_multipolygon_meta, num_point);

std::string geo_multipolygon_info::type_meta(geo_multipolygon const & row) {
    return processor_row::type_meta(row);
}

std::string geo_multipolygon_info::type_raw(geo_multipolygon const & row) {
    return to_string::type_raw(row.raw);
}
#else
static_col_name(geo_point_array_meta, SRID);
static_col_name(geo_point_array_meta, tag);
static_col_name(geo_point_array_meta, num_point);

std::string geo_point_array_info::type_meta(geo_point_array const & row) {
    return processor_row::type_meta(row);
}

std::string geo_point_array_info::type_raw(geo_point_array const & row) {
    return to_string::type_raw(row.raw);
}
#endif
//------------------------------------------------------------------------

static_col_name(geo_linesegment_meta, SRID);
static_col_name(geo_linesegment_meta, tag);
static_col_name(geo_linesegment_meta, first);
static_col_name(geo_linesegment_meta, second);

std::string geo_linesegment_info::type_meta(geo_linesegment const & row) {
    return processor_row::type_meta(row);
}

std::string geo_linesegment_info::type_raw(geo_linesegment const & row) {
    return to_string::type_raw(row.raw);
}

//------------------------------------------------------------------------

} // db
} // sdl
