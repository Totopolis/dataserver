// spatial_index.cpp
//
#include "common/common.h"
#include "spatial_index.h"
#include "system/page_info.h"

namespace sdl { namespace db {

//------------------------------------------------------------------------

template<> struct get_type_list<spatial_tree_row> : is_static {
    using type = spatial_tree_row_meta::type_list;
};

static_col_name(spatial_tree_row_meta, statusA);
static_col_name(spatial_tree_row_meta, cell_id);
static_col_name(spatial_tree_row_meta, pk0);
static_col_name(spatial_tree_row_meta, page);

std::string spatial_tree_row_info::type_meta(spatial_tree_row const & row) {
    return processor_row::type_meta(row);
}

std::string spatial_tree_row_info::type_raw(spatial_tree_row const & row) {
    return to_string::type_raw(row.raw);
}

//------------------------------------------------------------------------

static_col_name(spatial_page_row_meta, _0x00);
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
        A_STATIC_ASSERT_IS_POD(spatial_tree_key);
        A_STATIC_ASSERT_IS_POD(spatial_tree_row);
        A_STATIC_ASSERT_IS_POD(spatial_page_row);

        static_assert(sizeof(spatial_tree_key) == 13, "");
        static_assert(sizeof(spatial_tree_row) == 20, "");
        static_assert(sizeof(spatial_page_row) == 23, "");
    }
};
static unit_test s_test;
}
} // db
} // sdl
#endif //#if SV_DEBUG
