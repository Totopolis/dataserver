// file_header.cpp
//
#include "common/common.h"
#include "file_header.h"
#include "page_info.h"

namespace sdl { namespace db {

static_col_name(fileheader_field_meta, _0x00);
static_col_name(fileheader_field_meta, _0x02);
static_col_name(fileheader_field_meta, _0x04);
static_col_name(fileheader_field_meta, _0x08);
//static_col_name(fileheader_field_meta, _0x0C);
//static_col_name(fileheader_field_meta, NumberFields);
//static_col_name(fileheader_field_meta, FieldEndOffsets);

static_col_name(fileheader_row_meta, head);
static_col_name(fileheader_row_meta, field);

std::string fileheader_row_info::type_meta(fileheader_row const & row)
{
    struct to_string_ : to_string_with_head {
        using to_string_with_head::type; // allow type() methods from base class
        static std::string type(fileheader_field const & d) {
            std::stringstream ss;
            ss << "\n";
            impl::processor<fileheader_field_meta::type_list>::print(ss, &d);
            return ss.str();
        }
    };
    std::stringstream ss;
    impl::processor<fileheader_row_meta::type_list>::print(ss, &row,
        identity<to_string_>());
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
                    SDL_TRACE_FILE;
                    A_STATIC_ASSERT_IS_POD(fileheader_row);
                    static_assert(sizeof(fileheader_row) < page_head::body_size, "");
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG



