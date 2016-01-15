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

std::string sysallocunits_row_info::type_meta(sysallocunits_row const & row)
{
    std::stringstream ss;
    impl::processor<sysallocunits_row_meta::type_list>::print(ss, &row, 
        impl::identity<to_string_with_head>());
    return ss.str();
}

std::string sysallocunits_row_info::type_raw(sysallocunits_row const & row)
{
    return to_string::type_raw(row.raw);
}

const char * sysallocunits_row::type_name() const
{
    switch (data.type) {
    case 1: return "IN_ROW_DATA";
    case 2: return "LOB_DATA";
    case 3: return "ROW_OVERFLOW_DATA";
    default:
        SDL_ASSERT(0);
        return "";
    }
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

                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG