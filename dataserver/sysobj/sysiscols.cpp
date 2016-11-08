// sysiscols.cpp
//
#include "dataserver/common/common.h"
#include "dataserver/sysobj/sysiscols.h"
#include "dataserver/system/page_info.h"

namespace sdl { namespace db {

static_col_name(sysiscols_row_meta, head);
static_col_name(sysiscols_row_meta, idmajor);
static_col_name(sysiscols_row_meta, idminor);
static_col_name(sysiscols_row_meta, subid);
static_col_name(sysiscols_row_meta, status);
static_col_name(sysiscols_row_meta, intprop);
static_col_name(sysiscols_row_meta, tinyprop1);
static_col_name(sysiscols_row_meta, tinyprop2);
static_col_name(sysiscols_row_meta, tinyprop3);

std::string sysiscols_row_info::type_meta(sysiscols_row const & row)
{
    return processor_row::type_meta(row);
}

std::string sysiscols_row_info::type_raw(sysiscols_row const & row)
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
                    A_STATIC_ASSERT_IS_POD(sysiscols_row);
                    static_assert(sizeof(sysiscols_row) == 27, "");
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG