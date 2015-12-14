// sysidxstats.cpp
//
#include "common/common.h"
#include "sysidxstats.h"
#include "page_info.h"

namespace sdl { namespace db {

static_col_name(sysidxstats_row_meta, head);
static_col_name(sysidxstats_row_meta, id);
static_col_name(sysidxstats_row_meta, indid);
static_col_name(sysidxstats_row_meta, status);
static_col_name(sysidxstats_row_meta, intprop);
static_col_name(sysidxstats_row_meta, fillfact);
static_col_name(sysidxstats_row_meta, type);
static_col_name(sysidxstats_row_meta, tinyprop);
static_col_name(sysidxstats_row_meta, dataspace);
static_col_name(sysidxstats_row_meta, lobds);
static_col_name(sysidxstats_row_meta, rowset);
static_col_name(sysidxstats_row_meta, name);

std::string sysidxstats_row_info::type_meta(sysidxstats_row const & row)
{
    std::stringstream ss;
    impl::processor<sysidxstats_row_meta::type_list>::print(ss, &row, 
        impl::identity<to_string_with_head>());
    return ss.str();
}

std::string sysidxstats_row_info::type_raw(sysidxstats_row const & row)
{
    return to_string::type_raw(row.raw);
}

} // db
} // sdl

#if SDL_DEBUG
namespace sdl {
    namespace db {
        namespace {
            class unit_test {
            public:
                unit_test()
                {
                    SDL_TRACE(__FILE__);
                    A_STATIC_ASSERT_IS_POD(sysidxstats_row);
                    SDL_TRACE_2("sizeof(sysidxstats_row::data_type) = ", sizeof(sysidxstats_row::data_type));
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG