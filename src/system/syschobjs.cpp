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

std::string syschobjs_row_info::type_meta(syschobjs_row const & row)
{
    struct to_string_ : to_string {
        using to_string::type; // allow type() methods from base class
        static std::string type(datarow_head const & h) {
            std::stringstream ss;
            ss << "\n";
            ss << page_info::type_meta(h);
            return ss.str();
        }
    };
    std::stringstream ss;
    impl::processor<syschobjs_row_meta::type_list>::print(ss, &row,
        Type2Type<to_string_>());
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
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG