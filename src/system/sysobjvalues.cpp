// sysobjvalues.cpp
//
#include "common/common.h"
#include "sysobjvalues.h"
#include "page_info.h"

namespace sdl { namespace db {

static_col_name(sysobjvalues_meta, head);
static_col_name(sysobjvalues_meta, valclass);
static_col_name(sysobjvalues_meta, objid);
static_col_name(sysobjvalues_meta, subobjid);
static_col_name(sysobjvalues_meta, valnum);
//static_col_name(sysobjvalues_meta, value);

std::string sysobjvalues_row_info::type_meta(sysobjvalues_row const & row)
{
    /*struct to_string_ : to_string_with_head {
        using to_string_with_head::type; // allow type() methods from base class
        static std::string type(sysobjvalues_row::_48 const & value) {
            std::stringstream ss;
            ss << to_string::dump(&value, sizeof(value));
            return ss.str();
        }
    };*/
    std::stringstream ss;
    impl::processor<sysobjvalues_meta::type_list>::print(ss, &row, 
        impl::identity<to_string_with_head>());
    return ss.str();
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
                    SDL_TRACE(__FILE__);
                    A_STATIC_ASSERT_IS_POD(sysobjvalues_row);
                    //static_assert(sizeof(sysobjvalues_row::_48) == 6, "");
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG