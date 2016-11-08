// sysrowsets.cpp
//
#include "dataserver/sysobj/sysrowsets.h"
#include "dataserver/system/page_info.h"

namespace sdl { namespace db {

static_col_name(sysrowsets_row_meta, head);
static_col_name(sysrowsets_row_meta, rowsetid);
static_col_name(sysrowsets_row_meta, ownertype);
static_col_name(sysrowsets_row_meta, idmajor);
static_col_name(sysrowsets_row_meta, idminor);
static_col_name(sysrowsets_row_meta, numpart);
static_col_name(sysrowsets_row_meta, status);
static_col_name(sysrowsets_row_meta, fgidfs);
static_col_name(sysrowsets_row_meta, rcrows);
static_col_name(sysrowsets_row_meta, cmprlevel);
static_col_name(sysrowsets_row_meta, fillfact);
static_col_name(sysrowsets_row_meta, maxnullbit);
static_col_name(sysrowsets_row_meta, maxleaf);
static_col_name(sysrowsets_row_meta, maxint);
static_col_name(sysrowsets_row_meta, minleaf);
static_col_name(sysrowsets_row_meta, minint);

std::string sysrowsets_row_info::type_meta(sysrowsets_row const & row)
{
    return processor_row::type_meta(row);
}

std::string sysrowsets_row_info::type_raw(sysrowsets_row const & row)
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
                    A_STATIC_ASSERT_IS_POD(sysrowsets_row);
                    static_assert(sizeof(sysrowsets_row) < page_head::body_size, "");
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG