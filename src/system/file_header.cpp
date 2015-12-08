// file_header.cpp
//
#include "common/common.h"
#include "file_header.h"
#include "page_info.h"
#include <sstream>

namespace sdl { namespace db {

static_col_name(fileheader_row_meta, head);
static_col_name(fileheader_row_meta, NumberFields);
static_col_name(fileheader_row_meta, FieldEndOffsets);

std::string fileheader_row_info::type_meta(fileheader_row const & row)
{
    std::stringstream ss;
    impl::processor<fileheader_row_meta::type_list>::print(ss, &row,
        impl::identity<to_string_with_head>());
    return ss.str();
}

std::string fileheader_row_info::type_raw(fileheader_row const & row)
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
                    A_STATIC_ASSERT_IS_POD(fileheader_row);
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG



