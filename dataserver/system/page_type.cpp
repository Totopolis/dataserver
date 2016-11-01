// page_type.cpp
//
#include "common/common.h"
#include "page_type.h"
#include "common/time_util.h"
#include "utils/gregorian.hpp"
#include <cstddef>
#include <cstring>      // for memcmp
#include <algorithm>

namespace sdl { namespace db { namespace {

struct obj_code_name
{
    obj_code code;
    const char * name;
    obj_code_name(char c1, char c2, const char * n): name(n) {
        code.c[0] = c1;
        code.c[1] = c2;
    }
};

const obj_code_name OBJ_CODE_NAME[] = {
{ 'A', 'F' , "AGGREGATE_FUNCTION" },                    // 0
{ 'C', ' ' , "CHECK_CONSTRAINT" },                      // 1
{ 'D', ' ' , "DEFAULT_CONSTRAINT" },                    // 2
{ 'F', ' ' , "FOREIGN_KEY_CONSTRAINT" },                // 3
{ 'F', 'N' , "SQL_SCALAR_FUNCTION" },                   // 4
{ 'F', 'S' , "CLR_SCALAR_FUNCTION" },                   // 5
{ 'F', 'T' , "CLR_TABLE_VALUED_FUNCTION" },             // 6
{ 'I', 'F' , "SQL_INLINE_TABLE_VALUED_FUNCTION" },      // 7
{ 'I', 'T' , "INTERNAL_TABLE" },                        // 8
{ 'P', ' ' , "SQL_STORED_PROCEDURE" },                  // 9
{ 'P', 'C' , "CLR_STORED_PROCEDURE" },                  // 10
{ 'P', 'G' , "PLAN_GUIDE" },                            // 11
{ 'P', 'K' , "PRIMARY_KEY_CONSTRAINT" },                // 12
{ 'R', ' ' , "RULE" },                                  // 13
{ 'R', 'F' , "REPLICATION_FILTER_PROCEDURE" },          // 14
{ 'S', ' ' , "SYSTEM_TABLE" },                          // 15
{ 'S', 'N' , "SYNONYM" },                               // 16
{ 'S', 'Q' , "SERVICE_QUEUE" },                         // 17
{ 'T', 'A' , "CLR_TRIGGER" },                           // 18
{ 'T', 'F' , "SQL_TABLE_VALUED_FUNCTION" },             // 19
{ 'T', 'R' , "SQL_TRIGGER" },                           // 20
{ 'T', 'T' , "TYPE_TABLE" },                            // 21
{ 'U', ' ' , "USER_TABLE" },                            // 22
{ 'U', 'Q' , "UNIQUE_CONSTRAINT" },                     // 23
{ 'V', ' ' , "VIEW" },                                  // 24
{ 'X', ' ' , "EXTENDED_STORED_PROCEDURE" },             // 25
};

obj_code::type obj_code_type(obj_code const d) // linear search
{
    for (int i = 0; i < int(obj_code::type::_end); ++i) {
        if (OBJ_CODE_NAME[i].code.u == d.u) {
            return obj_code::type(i);
        }
    }
    SDL_ASSERT(0);
    return obj_code::type::_end;
}


struct scalartype_name {
    const scalartype::type t;
    const bool fixed;
    const char * const name;
    scalartype_name(scalartype::type _t, bool b, const char * s)
        : t(_t), fixed(b), name(s) {
        SDL_ASSERT(name);
    }
    operator scalartype::type() const { // to allow (operator <) with scalartype::type
        return this->t;
    }
};

const scalartype_name _SCALARTYPE_NAME[] = {
{ scalartype::t_none,             0, "" },
{ scalartype::t_image,            0, "image" },
{ scalartype::t_text,             0, "text" },
{ scalartype::t_uniqueidentifier, 1, "uniqueidentifier" },
{ scalartype::t_date,             1, "date" },
{ scalartype::t_time,             1, "time" },
{ scalartype::t_datetime2,        1, "datetime2" },
{ scalartype::t_datetimeoffset,   1, "datetimeoffset" },
{ scalartype::t_tinyint,          1, "tinyint" },
{ scalartype::t_smallint,         1, "smallint" },
{ scalartype::t_int,              1, "int" },
{ scalartype::t_smalldatetime,    1, "smalldatetime" },
{ scalartype::t_real,             1, "real" },
{ scalartype::t_money,            1, "money" },
{ scalartype::t_datetime,         1, "datetime" },
{ scalartype::t_float,            1, "float" },
{ scalartype::t_sql_variant,      0, "sql_variant" },
{ scalartype::t_ntext,            0, "ntext" }, // to be tested
{ scalartype::t_bit,              1, "bit" }, // to be tested
{ scalartype::t_decimal,          1, "decimal" },
{ scalartype::t_numeric,          1, "numeric" },
{ scalartype::t_smallmoney,       1, "smallmoney" },
{ scalartype::t_bigint,           1, "bigint" },
{ scalartype::t_hierarchyid,      0, "hierarchyid" },
{ scalartype::t_geometry,         0, "geometry" },
{ scalartype::t_geography,        0, "geography" },
{ scalartype::t_varbinary,        0, "varbinary" },
{ scalartype::t_varchar,          0, "varchar" },
{ scalartype::t_binary,           0, "binary" },
{ scalartype::t_char,             1, "char" },
{ scalartype::t_timestamp,        1, "timestamp" },
{ scalartype::t_nvarchar,         0, "nvarchar" },
{ scalartype::t_nchar,            1, "nchar" }, // to be tested
{ scalartype::t_xml,              0, "xml" },
{ scalartype::t_sysname,          0, "sysname" }
};

class FIND_SCALARTYPE_NAME {
    uint8 data[scalartype::_end];
    FIND_SCALARTYPE_NAME(){
        static_assert(A_ARRAY_SIZE(_SCALARTYPE_NAME) < 255, "");
        memset_zero(data);
        uint8 i = 0;
        for (auto const & x : _SCALARTYPE_NAME) {
            data[x.t] = i++;
        }
    }
public:
    static scalartype_name const & find(scalartype::type const t) {
        SDL_ASSERT(t != scalartype::t_none);
        static FIND_SCALARTYPE_NAME const obj;
        return _SCALARTYPE_NAME[obj.data[t]];
    }
};

struct complextype_name {
    const complextype::type t;
    const char * const name;
    complextype_name(complextype::type _t, const char * s): t(_t), name(s) {
        SDL_ASSERT(name);
    }
};

const complextype_name COMPLEXTYPE_NAME[] = {
{ complextype::row_overflow,        "row_overflow" },
{ complextype::blob_inline_root,    "blob_inline_root" },
{ complextype::sparse_vector,       "sparse_vector" },
{ complextype::forwarded,           "forwarded" }
};

struct idxtype_name {
    const idxtype::type t;
    const char * const name;
    idxtype_name(idxtype::type _t, const char * s): t(_t), name(s) {
        SDL_ASSERT(name);
    }
};

#if 0
const idxtype_name INDEXTYPE_NAME[] = {
{ idxtype::heap,          "HEAP" },
{ idxtype::clustered,     "CLUSTERED" },
{ idxtype::nonclustered,  "NONCLUSTERED" },
{ idxtype::xml,           "XML" },
{ idxtype::spatial,       "SPATIAL" }
};
#else
const idxtype_name INDEXTYPE_NAME[] = {
{ idxtype::heap,          "heap" },
{ idxtype::clustered,     "clustered" },
{ idxtype::nonclustered,  "nonclustered" },
{ idxtype::xml,           "xml" },
{ idxtype::spatial,       "spatial" }
};
#endif

} // namespace

// convert to number of seconds that have elapsed since 00:00:00 UTC, 1 January 1970
size_t datetime_t::get_unix_time() const
{
    SDL_ASSERT(unix_epoch());
    size_t result = (static_cast<size_t>(this->days) - static_cast<size_t>(u_date_diff)) * day_to_sec<1>::value;
    result += static_cast<size_t>(this->ticks) / 300;
    return result;
}

gregorian_t datetime_t::gregorian() const
{
    if (is_null()) {
        return {};
    }
    using calendar = date_time::gregorian_calendar;
    using ymd_type = calendar::ymd_type;
    const auto day_number = calendar::day_number(ymd_type(1900, 1, 1)) + this->days;
    const auto ymd = calendar::from_day_number(day_number);
    gregorian_t result; //uninitialized
    result.year = ymd.year;
    result.month = ymd.month;
    result.day = ymd.day;
    return result;
}

clocktime_t datetime_t::clocktime() const
{
    if (is_null()) {
        return {};
    }
    datetime_t src;
    src.ticks = this->ticks;
    src.days = datetime_t::u_date_diff;
    struct tm src_tm;
    if (time_util::safe_gmtime(src_tm, static_cast<time_t>(src.get_unix_time()))) {
        clocktime_t result; //uninitialized
        result.hour = src_tm.tm_hour;
        result.min = src_tm.tm_min;
        result.sec = src_tm.tm_sec;
        result.milliseconds = src.milliseconds();
        return result;
    }
    SDL_ASSERT(0);
    return {};
}

const char * obj_code::get_name(type const t)
{
    static_assert(A_ARRAY_SIZE(OBJ_CODE_NAME) == int(obj_code::type::_end), "");
    SDL_ASSERT(t != obj_code::type::_end);
    return OBJ_CODE_NAME[int(t)].name;
}

const char * obj_code::get_name(obj_code const d)
{
    const type t = obj_code_type(d); // linear search
    if (t != type::_end) {
        return obj_code::get_name(t);
    }
    return "";
}

obj_code obj_code::get_code(type const t)
{
    SDL_ASSERT(t != obj_code::type::_end);
    return OBJ_CODE_NAME[int(t)].code;
}

#if 0
struct tm {
int tm_sec;     /* seconds after the minute - [0,59] */
int tm_min;     /* minutes after the hour - [0,59] */
int tm_hour;    /* hours since midnight - [0,23] */
int tm_mday;    /* day of the month - [1,31] */
int tm_mon;     /* months since January - [0,11] */
int tm_year;    /* years since 1900 */
int tm_wday;    /* days since Sunday - [0,6] */
int tm_yday;    /* days since January 1 - [0,365] */
int tm_isdst;   /* daylight savings time flag */
}; 
#endif
datetime_t datetime_t::set_unix_time(size_t const val)
{
    datetime_t result = {};    
    time_t temp = static_cast<time_t>(val);
    struct tm tt;
    if (time_util::safe_gmtime(tt, temp)) {
        result.days = u_date_diff + int32(val / day_to_sec<1>::value);
        result.ticks = tt.tm_sec;
        result.ticks += tt.tm_min * min_to_sec<1>::value;
        result.ticks += tt.tm_hour * hour_to_sec<1>::value;
        result.ticks *= 300;
    }
    return result;
}

const char * scalartype::get_name(type const t)
{
    return FIND_SCALARTYPE_NAME::find(t).name;
}

bool scalartype::is_fixed(type const t)
{
    return FIND_SCALARTYPE_NAME::find(t).fixed;
}

//--------------------------------------------------------------

const char * complextype::get_name(type const t)
{
    for (auto const & s : COMPLEXTYPE_NAME) {
        if (s.t == t) {
            return s.name;
        }
    }
    SDL_ASSERT(0);
    return "";
}

const char * idxtype::get_name(type const t)
{
    for (auto const & s : INDEXTYPE_NAME) {
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
                    A_STATIC_ASSERT_IS_POD(guid_t);
                    A_STATIC_ASSERT_IS_POD(nchar_t);
                    A_STATIC_ASSERT_IS_POD(datetime_t);
                    A_STATIC_ASSERT_IS_POD(gregorian_t);
                    A_STATIC_ASSERT_IS_POD(clocktime_t);

                    static_assert(sizeof(pageType) == 1, "");
                    static_assert(sizeof(pageFileID) == 6, "");
                    static_assert(sizeof(recordID) == 8, "");
                    static_assert(sizeof(pageLSN) == 10, "");
                    static_assert(sizeof(pageXdesID) == 6, "");
                    static_assert(sizeof(guid_t) == 16, "");
                    static_assert(sizeof(nchar_t) == 2, "");
                    static_assert(sizeof(datetime_t) == 8, "");
                    static_assert(sizeof(bitmask8) == 1, "");   
                    static_assert(sizeof(var_mem_t<scalartype::t_varchar>) == sizeof(var_mem), "var_mem_t");

                    A_STATIC_ASSERT_IS_POD(auid_t);
                    static_assert(offsetof(auid_t, d.id) == 0x02, "");
                    static_assert(offsetof(auid_t, d.hi) == 0x06, "");
                    static_assert(sizeof(auid_t) == 8, "");
                    {
                        uint16 min_u, max_u;
                        min_u = max_u = obj_code::get_code(obj_code::type::AGGREGATE_FUNCTION).u;
                        for (int i = 0; i < int(obj_code::type::_end); ++i) {
                            const obj_code::type t = obj_code::type(i);
                            const obj_code code = obj_code::get_code(t);
                            auto s1 = obj_code::get_name(t);
                            auto s2 = obj_code::get_name(code);
                            SDL_ASSERT(s1 == s2);
                            if (min_u > code.u) min_u = code.u;
                            if (max_u < code.u) max_u = code.u;
                        }
                        SDL_ASSERT((max_u - min_u) == 13329);
                    }
                    A_STATIC_ASSERT_IS_POD(obj_code);
                    A_STATIC_ASSERT_IS_POD(schobj_id);
                    A_STATIC_ASSERT_IS_POD(nsid_id);
                    A_STATIC_ASSERT_IS_POD(index_id);
                    A_STATIC_ASSERT_IS_POD(scalarlen);
                    A_STATIC_ASSERT_IS_POD(scalartype);
                    A_STATIC_ASSERT_IS_POD(complextype);
                    A_STATIC_ASSERT_IS_POD(idxtype);
                    A_STATIC_ASSERT_IS_POD(idxstatus);
                    A_STATIC_ASSERT_IS_POD(column_xtype);
                    A_STATIC_ASSERT_IS_POD(column_id);
                    A_STATIC_ASSERT_IS_POD(iscolstatus);
                    A_STATIC_ASSERT_IS_POD(numeric9);
                    A_STATIC_ASSERT_IS_POD(numeric_t<5>);
                    A_STATIC_ASSERT_IS_POD(decimal5);
                    A_STATIC_ASSERT_IS_POD(pair_key<char>);
                    static_assert(sizeof(pair_key<char>) == 2, "");

                    static_assert(sizeof(obj_code) == 2, "");
                    static_assert(sizeof(schobj_id) == 4, "");
                    static_assert(sizeof(nsid_id) == 4, "");
                    static_assert(sizeof(index_id) == 4, "");
                    static_assert(sizeof(scalarlen) == 2, "");
                    static_assert(sizeof(scalartype) == 4, "");
                    static_assert(sizeof(complextype) == 2, "");
                    static_assert(sizeof(idxtype) == 1, "");
                    static_assert(sizeof(idxstatus) == 4, "");
                    static_assert(sizeof(column_xtype) == 1, "");
                    static_assert(sizeof(column_id) == 4, "");
                    static_assert(sizeof(iscolstatus) == 4, "");
                    static_assert(sizeof(numeric9) == 9, "");
                    static_assert(sizeof(numeric_t<5>) == 5, "");
                    static_assert(sizeof(decimal5) == 5, "");

                    A_STATIC_ASSERT_IS_POD(pfs_byte);
                    static_assert(sizeof(pfs_byte) == 1, "");
                    static_assert(sizeof(pfs_byte::bitfields) == 1, "");
                    static_assert(make_break_or_continue(false) == bc::break_, "");
                    static_assert(make_break_or_continue(true) == bc::continue_, "");
                    {
                        pfs_byte b{};
                        b.set_fullness(pfs_full::PCT_FULL_100);
                        SDL_ASSERT(b.fullness() == pfs_full::PCT_FULL_100);
                    }
                    static_assert(pageType::size == 21, "");
                    static_assert(dataType::size == 4, "");

                    A_STATIC_ASSERT_IS_POD(smalldatetime_t);
                    static_assert(sizeof(smalldatetime_t) == 4, "");

                    SDL_ASSERT(obj_code::get_code(obj_code::type::USER_TABLE).name()[0]);
                    SDL_ASSERT(scalartype{scalartype::t_int}.name()[0]);
                    SDL_ASSERT(complextype{complextype::row_overflow}.name()[0]);
                    SDL_ASSERT(idxtype{idxtype::clustered}.name()[0]);
                    {
                        recordID x{};
                        recordID y{};
                        SDL_ASSERT(!(x < y));
                        x.id.pageId = 1;
                        SDL_ASSERT(x.id.compare(y.id) == 1);
                        SDL_ASSERT(y.id.compare(x.id) == -1);
                        SDL_ASSERT(x.id.compare(x.id) == 0);
                        SDL_ASSERT(y < x);
                    }
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG

