// iam_page.cpp
//
#include "common/common.h"
#include "iam_page.h"
#include "page_info.h"

namespace sdl { namespace db {

static_col_name(iam_page_row_meta, head);
static_col_name(iam_page_row_meta, sequenceNumber);
static_col_name(iam_page_row_meta, status);
static_col_name(iam_page_row_meta, objectId);
static_col_name(iam_page_row_meta, indexId);
static_col_name(iam_page_row_meta, page_count);
static_col_name(iam_page_row_meta, start_pg);
static_col_name(iam_page_row_meta, slot_pg);

std::string iam_page_row_info::type_meta(iam_page_row const & row)
{
    return processor_row::type_meta(row);
}

std::string iam_page_row_info::type_raw(iam_page_row const & row)
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
                    SDL_TRACE_FILE;
                    A_STATIC_ASSERT_IS_POD(iam_page_row);   
                    static_assert(offsetof(iam_page_row, data._0x08) == 0x08, "");
                    static_assert(offsetof(iam_page_row, data._0x14) == 0x14, "");
                    static_assert(offsetof(iam_page_row, data._0x27) == 0x27, "");
                    static_assert(offsetof(iam_page_row, data.slot_pg) == 0x2E, "");
                    static_assert(sizeof(iam_page_row::data_type) == 94, "");
                    static_assert(sizeof(iam_page_row::data_type().slot_pg) == 48, "");
                    A_STATIC_ASSERT_IS_POD(pfs_page_row);
                    static_assert(pfs_page_row::body_size * page_head::page_size == 8088 * 8192, "");
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG