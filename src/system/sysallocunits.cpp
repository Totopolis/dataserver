// sysallocunits.cpp
//
#include "common/common.h"
#include "sysallocunits.h"
#include "page_info.h"

namespace sdl { namespace db {

static_col_name(sysallocunits_row_meta, head);
static_col_name(sysallocunits_row_meta, auid);
static_col_name(sysallocunits_row_meta, type);
static_col_name(sysallocunits_row_meta, ownerid);
static_col_name(sysallocunits_row_meta, status);
static_col_name(sysallocunits_row_meta, fgid);
static_col_name(sysallocunits_row_meta, pgfirst);
static_col_name(sysallocunits_row_meta, pgroot);
static_col_name(sysallocunits_row_meta, pgfirstiam);
static_col_name(sysallocunits_row_meta, pcused);
static_col_name(sysallocunits_row_meta, pcdata);
static_col_name(sysallocunits_row_meta, pcreserved);
static_col_name(sysallocunits_row_meta, dbfragid);
//
static_col_name(iam_page_row_meta, head);
static_col_name(iam_page_row_meta, seq);
static_col_name(iam_page_row_meta, status);
static_col_name(iam_page_row_meta, objectID);
static_col_name(iam_page_row_meta, indexID);
static_col_name(iam_page_row_meta, pageCount);
static_col_name(iam_page_row_meta, startPage);
static_col_name(iam_page_row_meta, slot);

std::string sysallocunits_row_info::type_meta(sysallocunits_row const & row)
{
    return processor_row::type_meta(row);
}

std::string sysallocunits_row_info::type_raw(sysallocunits_row const & row)
{
    return to_string::type_raw(row.raw);
}

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
                    
                    A_STATIC_ASSERT_IS_POD(sysallocunits_row);                    
                    static_assert(sizeof(sysallocunits_row) < page_head::body_size, "");
                    
                    A_STATIC_ASSERT_IS_POD(iam_page_row);   
                    static_assert(offsetof(iam_page_row, data._0x04) == 0x04 + sizeof(row_head), "");
                    static_assert(offsetof(iam_page_row, data._0x10) == 0x10 + sizeof(row_head), "");
                    static_assert(offsetof(iam_page_row, data._0x23) == 0x23 + sizeof(row_head), "");
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG