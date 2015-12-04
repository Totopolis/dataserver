// sysallocunits.cpp
//
#include "common/common.h"
#include "sysallocunits.h"
#include "page_info.h"

namespace sdl { namespace db {

static_col_name(sysallocunits_row_meta, head);
static_col_name(sysallocunits_row_meta, auid);
static_col_name(sysallocunits_row_meta, type);
static_col_name(sysallocunits_row_meta, ownerid);
static_col_name(sysallocunits_row_meta, status);
static_col_name(sysallocunits_row_meta, fgid);
static_col_name(sysallocunits_row_meta, pgfirst);
static_col_name(sysallocunits_row_meta, pgroot);
static_col_name(sysallocunits_row_meta, pgfirstiam);
static_col_name(sysallocunits_row_meta, pcused);
static_col_name(sysallocunits_row_meta, pcdata);
static_col_name(sysallocunits_row_meta, pcreserved);
static_col_name(sysallocunits_row_meta, dbfragid);

std::string sysallocunits_row_info::type_meta(sysallocunits_row const & row)
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
    impl::processor<sysallocunits_row_meta::type_list>::print(ss, &row, 
        impl::identity<to_string_>());
    return ss.str();
}

std::string sysallocunits_row_info::type_raw(sysallocunits_row const & row)
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
                    
                    A_STATIC_ASSERT_IS_POD(sysallocunits_row);
                    static_assert(sizeof(sysallocunits_row) < page_head::body_size, "");

                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG