// sysschobjs.cpp
//
#include "common/common.h"
#include "sysschobjs.h"
#include "page_info.h"

namespace sdl { namespace db {

static_col_name(sysschobjs_row_meta, head);
static_col_name(sysschobjs_row_meta, id);
static_col_name(sysschobjs_row_meta, nsid);
static_col_name(sysschobjs_row_meta, nsclass);
static_col_name(sysschobjs_row_meta, status);
static_col_name(sysschobjs_row_meta, type);
static_col_name(sysschobjs_row_meta, pid);
static_col_name(sysschobjs_row_meta, pclass);
static_col_name(sysschobjs_row_meta, intprop);
static_col_name(sysschobjs_row_meta, created);
static_col_name(sysschobjs_row_meta, modified);
static_col_name(sysschobjs_row_meta, name);

std::string sysschobjs_row_info::type_meta(sysschobjs_row const & row)
{
    return processor_row::type_meta(row);
}

std::string sysschobjs_row_info::type_raw(sysschobjs_row const & row)
{
    return to_string::type_raw(row.raw);
}

std::string sysschobjs_row_info::col_name(sysschobjs_row const & row)
{
    return to_string::type_nchar(row.data.head, sysschobjs_row_meta::name::offset);
}

/*std::string sysschobjs_row::col_name() const
{
    return sysschobjs_row_info::col_name(*this);
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
                    A_STATIC_ASSERT_IS_POD(sysschobjs_row);
                    static_assert(sizeof(sysschobjs_row) < page_head::body_size, "");
                    static_assert(sizeof(sysschobjs_row) == 44, "");
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG