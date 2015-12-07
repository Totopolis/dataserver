// file_header.cpp
//
#include "common/common.h"
#include "file_header.h"
#include "page_info.h"
#include <sstream>

namespace sdl { namespace db {

static_col_name(file_header_row_meta, NumberFields);

std::string file_header_row_info::type(file_header_row const & row)
{
    std::stringstream ss;
    impl::processor<file_header_row_meta::type_list>::print(ss, &row);
    return ss.str();
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
                    A_STATIC_ASSERT_IS_POD(file_header_row);
                    static_assert(offsetof(file_header_row, data.NumberFields) == 0x10, "");
                    static_assert(offsetof(file_header_row, data.FieldEndOffsets) == 0x12, "");
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG



