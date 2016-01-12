// syscolpars.cpp
//
#include "common/common.h"
#include "syscolpars.h"
#include "page_info.h"

namespace sdl { namespace db {

static_col_name(syscolpars_row_meta, head);
static_col_name(syscolpars_row_meta, id);
static_col_name(syscolpars_row_meta, number);
static_col_name(syscolpars_row_meta, colid);
static_col_name(syscolpars_row_meta, xtype);
static_col_name(syscolpars_row_meta, utype);
static_col_name(syscolpars_row_meta, length);
static_col_name(syscolpars_row_meta, prec);
static_col_name(syscolpars_row_meta, scale);
static_col_name(syscolpars_row_meta, collationid);
static_col_name(syscolpars_row_meta, status);
static_col_name(syscolpars_row_meta, maxinrow);
static_col_name(syscolpars_row_meta, xmlns);
static_col_name(syscolpars_row_meta, dflt);
static_col_name(syscolpars_row_meta, chk);
//
static_col_name(syscolpars_row_meta, name);

std::string syscolpars_row_info::type_meta(syscolpars_row const & row)
{
    std::stringstream ss;
    impl::processor<syscolpars_row_meta::type_list>::print(ss, &row, 
        impl::identity<to_string_with_head>());
    return ss.str();
}

std::string syscolpars_row_info::type_raw(syscolpars_row const & row)
{
    return to_string::type_raw(row.raw);
}

std::string syscolpars_row_info::col_name(syscolpars_row const & row)
{
    return to_string::type_nchar(row.data.head, syscolpars_row_meta::name::offset);
}

std::string syscolpars_row::col_name() const
{
    return syscolpars_row_info::col_name(*this);
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
                    A_STATIC_ASSERT_IS_POD(syscolpars_row);
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG