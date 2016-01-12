// sysscalartypes.cpp
//
#include "common/common.h"
#include "sysscalartypes.h"
#include "page_info.h"

namespace sdl { namespace db { namespace {

struct scalartype_name {
    scalartype t;
    const char * name;
    scalartype_name(scalartype _t, const char * s) : t(_t), name(s) {}
};

const scalartype_name SCALARTYPE[] = {
{ scalartype::t_image, "image" },
{ scalartype::t_text, "text" },
{ scalartype::t_uniqueidentifier, "uniqueidentifier" },
{ scalartype::t_date, "date" },
{ scalartype::t_time, "time" },
{ scalartype::t_datetime2, "datetime2" },
{ scalartype::t_datetimeoffset, "datetimeoffset" },
{ scalartype::t_tinyint, "tinyint" },
{ scalartype::t_smallint, "smallint" },
{ scalartype::t_int, "int" },
{ scalartype::t_smalldatetime, "smalldatetime" },
{ scalartype::t_real, "real" },
{ scalartype::t_money, "money" },
{ scalartype::t_datetime, "datetime" },
{ scalartype::t_float, "float" },
{ scalartype::t_sql_variant, "sql_variant" },
{ scalartype::t_ntext, "ntext" },
{ scalartype::t_bit, "bit" },
{ scalartype::t_decimal, "decimal" },
{ scalartype::t_numeric, "numeric" },
{ scalartype::t_smallmoney, "smallmoney" },
{ scalartype::t_bigint, "bigint" },
{ scalartype::t_hierarchyid, "hierarchyid" },
{ scalartype::t_geometry, "geometry" },
{ scalartype::t_geography, "geography" },
{ scalartype::t_varbinary, "varbinary" },
{ scalartype::t_varchar, "varchar" },
{ scalartype::t_binary, "binary" },
{ scalartype::t_char, "char" },
{ scalartype::t_timestamp, "timestamp" },
{ scalartype::t_nvarchar, "nvarchar" },
{ scalartype::t_nchar, "nchar" },
{ scalartype::t_xml, "xml" },
{ scalartype::t_sysname, "sysname" }
};

} // namespace

static_col_name(sysscalartypes_row_meta, head);
static_col_name(sysscalartypes_row_meta, id);
static_col_name(sysscalartypes_row_meta, schid);
static_col_name(sysscalartypes_row_meta, xtype);
static_col_name(sysscalartypes_row_meta, length);
static_col_name(sysscalartypes_row_meta, prec);
static_col_name(sysscalartypes_row_meta, scale);
static_col_name(sysscalartypes_row_meta, collationid);
static_col_name(sysscalartypes_row_meta, status);
static_col_name(sysscalartypes_row_meta, created);
static_col_name(sysscalartypes_row_meta, modified);
static_col_name(sysscalartypes_row_meta, dflt);
static_col_name(sysscalartypes_row_meta, chk);
static_col_name(sysscalartypes_row_meta, name);

std::string sysscalartypes_row_info::type_meta(sysscalartypes_row const & row)
{
    std::stringstream ss;
    impl::processor<sysscalartypes_row_meta::type_list>::print(ss, &row, 
        impl::identity<to_string_with_head>());
    return ss.str();
}

std::string sysscalartypes_row_info::type_raw(sysscalartypes_row const & row)
{
    return to_string::type_raw(row.raw);
}

std::string sysscalartypes_row_info::col_name(sysscalartypes_row const & row)
{
    return to_string::type_nchar(row.data.head, sysscalartypes_row_meta::name::offset);
}

std::string sysscalartypes_row::col_name() const
{
    return sysscalartypes_row_info::col_name(*this);
}

scalartype scalartype_info::find(uint32 const t)
{
    static_assert(A_ARRAY_SIZE(SCALARTYPE) == 34, "");
    for (auto & s : SCALARTYPE) {
        if (s.t == static_cast<scalartype>(t)) {
            return s.t;
        }
    }
    SDL_ASSERT(0);
    return scalartype::t_none;
}

std::string scalartype_info::type(scalartype const t)
{
    for (auto & s : SCALARTYPE) {
        if (s.t == t) {
            return s.name;
        }
    }
    SDL_ASSERT(0);
    return "";
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
                    A_STATIC_ASSERT_IS_POD(sysscalartypes_row);
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG