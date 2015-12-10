// syschobjs.cpp
//
#include "common/common.h"
#include "syschobjs.h"
#include "page_info.h"

namespace sdl { namespace db {

static_col_name(syschobjs_row_meta, head);
static_col_name(syschobjs_row_meta, id);
static_col_name(syschobjs_row_meta, nsid);
static_col_name(syschobjs_row_meta, nsclass);
static_col_name(syschobjs_row_meta, status);
static_col_name(syschobjs_row_meta, type);
static_col_name(syschobjs_row_meta, pid);
static_col_name(syschobjs_row_meta, pclass);
static_col_name(syschobjs_row_meta, intprop);
static_col_name(syschobjs_row_meta, created);
static_col_name(syschobjs_row_meta, modified);
//
static_col_name(syschobjs_row_meta, name);

std::string syschobjs_row_info::type_meta(syschobjs_row const & row)
{
    std::stringstream ss;
    impl::processor<syschobjs_row_meta::type_list>::print(ss, &row,
        impl::identity<to_string_with_head>());
    return ss.str();
}

std::string syschobjs_row_info::type_raw(syschobjs_row const & row)
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
                    A_STATIC_ASSERT_IS_POD(syschobjs_row);
                    A_STATIC_ASSERT_IS_POD(obj_code);
                    static_assert(sizeof(obj_code) == 2, "");
                    static_assert(sizeof(syschobjs_row) < page_head::body_size, "");
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG