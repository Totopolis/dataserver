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

namespace {

struct obj_code_name
{
    obj_code code;
    const char * name;
    obj_code_name(char c1, char c2, const char * n) : name(n) {
        code.c[0] = c1;
        code.c[1] = c2;
    }
};

const obj_code_name OBJ_CODE_NAME[] = {
{ 'A', 'F' , "AGGREGATE_FUNCTION" },
{ 'C', ' ' , "CHECK_CONSTRAINT" },
{ 'D', ' ' , "DEFAULT_CONSTRAINT" },
{ 'F', ' ' , "FOREIGN_KEY_CONSTRAINT" },
{ 'F', 'N' , "SQL_SCALAR_FUNCTION" },
{ 'F', 'S' , "CLR_SCALAR_FUNCTION" },
{ 'F', 'T' , "CLR_TABLE_VALUED_FUNCTION" },
{ 'I', 'F' , "SQL_INLINE_TABLE_VALUED_FUNCTION" },
{ 'I', 'T' , "INTERNAL_TABLE" },
{ 'P', ' ' , "SQL_STORED_PROCEDURE" },
{ 'P', 'C' , "CLR_STORED_PROCEDURE" },
{ 'P', 'G' , "PLAN_GUIDE" },
{ 'P', 'K' , "PRIMARY_KEY_CONSTRAINT" },
{ 'R', ' ' , "RULE" },
{ 'R', 'F' , "REPLICATION_FILTER_PROCEDURE" },
{ 'S', ' ' , "SYSTEM_TABLE" },
{ 'S', 'N' , "SYNONYM" },
{ 'S', 'Q' , "SERVICE_QUEUE" },
{ 'T', 'A' , "CLR_TRIGGER" },
{ 'T', 'F' , "SQL_TABLE_VALUED_FUNCTION" },
{ 'T', 'R' , "SQL_TRIGGER" },
{ 'T', 'T' , "TYPE_TABLE" },
{ 'U', ' ' , "USER_TABLE" },
{ 'U', 'Q' , "UNIQUE_CONSTRAINT" },
{ 'V', ' ' , "VIEW" },
{ 'X', ' ' , "EXTENDED_STORED_PROCEDURE" },
};

} // namespace

const char * syschobjs_row_info::code_name(obj_code const & d)
{
    static_assert(A_ARRAY_SIZE(OBJ_CODE_NAME) == 26, "");
    for (auto & it : OBJ_CODE_NAME) {
        if (it.code.u == d.u)
            return it.name;
    }
    SDL_WARNING(0);
    return "";
}

std::string syschobjs_row_info::type_meta(syschobjs_row const & row)
{
    struct to_string_ : to_string_with_head {
        using to_string_with_head::type; // allow type() methods from base class
        static std::string type(obj_code const & d) {
            std::stringstream ss;
            ss << d.u << " \"" << d.c[0] << d.c[1] << "\"";
            ss << " " << code_name(d);
            return ss.str();
        }
    };
    std::stringstream ss;
    impl::processor<syschobjs_row_meta::type_list>::print(ss, &row, 
        impl::identity<to_string_>());
    return ss.str();
}

std::string syschobjs_row_info::type_raw(syschobjs_row const & row)
{
    return to_string::type_raw(row.raw);
}

/*FIXME: name (name) - the name of the object, Unicode, variable length. 
The (name, schema) must be unique among all objects in a database.*/

nchar_range
syschobjs_row::var_name(syschobjs_row const * row) // FIXME: will be improved
{
    db::variable_array const var(row);
    if (!var.empty()) {
        const size_t n = var[0] / sizeof(nchar_t); // length in nchar_t
        if (n) {
            const nchar_t nzero[2] = { {0x0030}, {0x0030} };
            nchar_t const * const begin = var.end_t<nchar_t>(); // at Col1 data
            nchar_range name(begin, begin + n);
            if (nchar_t const * p = reverse_find(name, nzero)) {
                SDL_ASSERT(name.first <= p);
                name.second = p;
                return name;
            }
            return name;
        }
        SDL_ASSERT(!"var_name");
    }
    return nchar_range();
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