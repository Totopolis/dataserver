// sysallocunits.cpp
//
#include "common/common.h"
#include "sysallocunits.h"
#include "page_info.h"

namespace sdl { namespace db {

static_col_name(sysallocunits_row_meta, statusA);
static_col_name(sysallocunits_row_meta, statusB);
static_col_name(sysallocunits_row_meta, fixedlen);

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
        static std::string type(auid_t const & id) {
            std::stringstream ss;
            ss << int(id.d.lo) << ":"
                << int(id.d.id) << ":"
                << int(id.d.hi)
                << " ("
                << "0x" << std::uppercase << std::hex
                << id._64
                << ")"
                << std::dec
                << " ("
                << id._64
                << ")";
            auto s = ss.str();
            return s;
        }
    };
    typedef std::identity<to_string_> format;
    std::stringstream ss;
    impl::processor<sysallocunits_row_meta::type_list>::print(ss, &row, format());
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
                    A_STATIC_ASSERT_IS_POD(auid_t);
                    
                    static_assert(sizeof(sysallocunits_row) < page_head::body_size, "");
                    static_assert(offsetof(auid_t, d.id) == 0x02, "");
                    static_assert(offsetof(auid_t, d.hi) == 0x06, "");
                    static_assert(sizeof(auid_t) == 8, "");
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG