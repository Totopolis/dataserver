// sysobjvalues.cpp
//
#include "dataserver/sysobj/sysobjvalues.h"
#include "dataserver/system/page_info.h"

namespace sdl { namespace db {

static_col_name(sysobjvalues_row_meta, head);
static_col_name(sysobjvalues_row_meta, valclass);
static_col_name(sysobjvalues_row_meta, objid);
static_col_name(sysobjvalues_row_meta, subobjid);
static_col_name(sysobjvalues_row_meta, valnum);
//static_col_name(sysobjvalues_row_meta, value);

std::string sysobjvalues_row_info::type_meta(sysobjvalues_row const & row)
{
    return processor_row::type_meta(row);
}

std::string sysobjvalues_row_info::type_raw(sysobjvalues_row const & row)
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
                    A_STATIC_ASSERT_IS_POD(sysobjvalues_row);
                    //static_assert(sizeof(sysobjvalues_row::_48) == 6, "");
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SDL_DEBUG
