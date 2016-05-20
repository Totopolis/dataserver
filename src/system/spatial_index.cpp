// spatial_index.cpp
//
#include "common/common.h"
#include "spatial_index.h"
#include "system/page_info.h"

namespace sdl { namespace db {

static_col_name(spatial_page_row_meta, cell_id);
static_col_name(spatial_page_row_meta, pk0);
static_col_name(spatial_page_row_meta, cell_attr);
static_col_name(spatial_page_row_meta, SRID);

std::string spatial_page_row_info::type_meta(spatial_page_row const & row) {
    return processor_row::type_meta(row);
}

std::string spatial_page_row_info::type_raw(spatial_page_row const & row) {
    return to_string::type_raw(row.raw);
}

//------------------------------------------------------------------------

static_col_name(geo_point_meta, SRID);
static_col_name(geo_point_meta, _0x04);
static_col_name(geo_point_meta, latitude);
static_col_name(geo_point_meta, longitude);

std::string geo_point_info::type_meta(geo_point const & row) {
    return processor_row::type_meta(row);
}

std::string geo_point_info::type_raw(geo_point const & row) {
    return to_string::type_raw(row.raw);
}

//------------------------------------------------------------------------

static_col_name(geo_multipolygon_meta, SRID);
static_col_name(geo_multipolygon_meta, _0x04);
static_col_name(geo_multipolygon_meta, num_point);

std::string geo_multipolygon_info::type_meta(geo_multipolygon const & row) {
    return processor_row::type_meta(row);
}

std::string geo_multipolygon_info::type_raw(geo_multipolygon const & row) {
    return to_string::type_raw(row.raw);
}

//------------------------------------------------------------------------

} // db
} // sdl

#if SDL_DEBUG
namespace sdl { namespace db { namespace {
class unit_test {
public:
    unit_test()
    {
        A_STATIC_ASSERT_IS_POD(spatial_root_row_20);
        A_STATIC_ASSERT_IS_POD(spatial_root_row_23);
        A_STATIC_ASSERT_IS_POD(spatial_node_row);
        A_STATIC_ASSERT_IS_POD(spatial_page_row);
        A_STATIC_ASSERT_IS_POD(geo_point);
        A_STATIC_ASSERT_IS_POD(geo_multipolygon);

        static_assert(sizeof(spatial_root_row_20) == 20, "");
        static_assert(sizeof(spatial_root_row_23) == 23, "");
        static_assert(sizeof(spatial_node_row) == 23, "");
        static_assert(sizeof(spatial_page_row) == 26, "");
        static_assert(sizeof(geo_point) == 22, "");
        static_assert(sizeof(geo_multipolygon) == 26, "");
        {
            geo_multipolygon test{};
            SDL_ASSERT(test.mem_size() == sizeof(geo_multipolygon)-sizeof(spatial_point));
            test.data.num_point = 1;
            SDL_ASSERT(test.mem_size() == sizeof(geo_multipolygon));
        }
    }
};
static unit_test s_test;
}
} // db
} // sdl
#endif //#if SV_DEBUG
