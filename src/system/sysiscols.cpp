// sysiscols.cpp
//
#include "common/common.h"
#include "sysiscols.h"
#include "page_info.h"

namespace sdl { namespace db {

static_col_name(sysiscols_meta, head);
static_col_name(sysiscols_meta, idmajor);
static_col_name(sysiscols_meta, idminor);
static_col_name(sysiscols_meta, subid);
static_col_name(sysiscols_meta, status);
static_col_name(sysiscols_meta, intprop);
static_col_name(sysiscols_meta, tinyprop1);
static_col_name(sysiscols_meta, tinyprop2);

std::string sysiscols_row_info::type_meta(sysiscols_row const & row)
{
    std::stringstream ss;
    impl::processor<sysiscols_meta::type_list>::print(ss, &row, 
        impl::identity<to_string_with_head>());
    return ss.str();
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
                    SDL_TRACE(__FILE__);
                    A_STATIC_ASSERT_IS_POD(sysiscols_row);
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG