// sysscalartypes.cpp
//
#include "common/common.h"
#include "sysscalartypes.h"
#include "page_info.h"

namespace sdl { namespace db {

static_col_name(sysscalartypes_row_meta, head);
static_col_name(sysscalartypes_row_meta, id);
static_col_name(sysscalartypes_row_meta, schid);
static_col_name(sysscalartypes_row_meta, xtype);
static_col_name(sysscalartypes_row_meta, length);
static_col_name(sysscalartypes_row_meta, prec);
static_col_name(sysscalartypes_row_meta, scale);
static_col_name(sysscalartypes_row_meta, collationid);
static_col_name(sysscalartypes_row_meta, status);
static_col_name(sysscalartypes_row_meta, created);
static_col_name(sysscalartypes_row_meta, modified);
static_col_name(sysscalartypes_row_meta, dflt);
static_col_name(sysscalartypes_row_meta, chk);
static_col_name(sysscalartypes_row_meta, name);

std::string sysscalartypes_row_info::type_meta(sysscalartypes_row const & row)
{
    return processor_row::type_meta(row);
}

std::string sysscalartypes_row_info::type_raw(sysscalartypes_row const & row)
{
    return to_string::type_raw(row.raw);
}

std::string sysscalartypes_row_info::col_name(sysscalartypes_row const & row)
{
    return to_string::type_nchar(row.data.head, sysscalartypes_row_meta::name::offset);
}

/*std::string sysscalartypes_row::col_name() const
{
    return sysscalartypes_row_info::col_name(*this);
}*/

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
                    A_STATIC_ASSERT_IS_POD(sysscalartypes_row);
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG