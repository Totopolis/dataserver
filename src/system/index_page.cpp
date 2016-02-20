// index_page.cpp
//
#include "common/common.h"
#include "index_page.h"
#include "page_info.h"

namespace sdl { namespace db {

struct index_page_row_meta: is_static {

    typedef_col_type_n(index_page_row, statusA);
    typedef_col_type_n(index_page_row, key);
    typedef_col_type_n(index_page_row, page);

    typedef TL::Seq<
        statusA
        ,key
        ,page
    >::Type type_list;
};

static_col_name(index_page_row_meta, statusA);
static_col_name(index_page_row_meta, key);
static_col_name(index_page_row_meta, page);

template<> struct get_type_list<index_page_row> : is_static
{
    using type = index_page_row_meta::type_list;
};

std::string index_page_row_info::type_meta(index_page_row const & row)
{
    return processor_row::type_meta(row);
}

std::string index_page_row_info::type_raw(index_page_row const & row)
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
                    A_STATIC_ASSERT_IS_POD(index_page_row);
                    static_assert(sizeof(index_page_row) == 15, "");
                    static_assert(sizeof(index_page_row) == (7 + sizeof(index_page_row::key_type)), "");
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG