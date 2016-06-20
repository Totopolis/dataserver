// spatial_index.cpp
//
#include "common/common.h"
#include "spatial_index.h"
#include "system/page_info.h"

namespace sdl { namespace db {

static_col_name(spatial_root_row_meta, statusA);
static_col_name(spatial_root_row_meta, cell_id);
static_col_name(spatial_root_row_meta, pk0);
static_col_name(spatial_root_row_meta, page);

std::string spatial_root_row_info::type_meta(spatial_root_row const & row) {
    return processor_row::type_meta(row);
}

std::string spatial_root_row_info::type_raw(spatial_root_row const & row) {
    return to_string::type_raw(row.raw);
}

//------------------------------------------------------------------------

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

} // db
} // sdl

#if SDL_DEBUG
namespace sdl { namespace db { namespace {
class unit_test {
public:
    unit_test()
    {
        A_STATIC_ASSERT_IS_POD(spatial_root_row);
        A_STATIC_ASSERT_IS_POD(spatial_node_row);
        A_STATIC_ASSERT_IS_POD(spatial_page_row);

        static_assert(sizeof(spatial_root_row) == 20, "");
        static_assert(sizeof(spatial_node_row) == 23, "");
        static_assert(sizeof(spatial_page_row) == 26, "");
    }
};
static unit_test s_test;
}
} // db
} // sdl
#endif //#if SV_DEBUG
