// page_type.cpp
//
#include "common/common.h"
#include "page_type.h"
#include <time.h>       /* time_t, struct tm, time, localtime, strftime */
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
        : t(_t), fixed(b), name(s)
    {
        SDL_ASSERT(name);
    }
};

const scalartype_name SCALARTYPE_NAME[] = {
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

inline bool operator < (scalartype_name const & lh, scalartype_name const & rh) { return lh.t < rh.t; }
inline bool operator < (scalartype_name const & lh, scalartype::type rh) { return lh.t < rh; }
inline bool operator < (scalartype::type lh, scalartype_name const & rh) { return lh < rh.t; }

const scalartype_name * find_scalartype(scalartype::type const t)
{
#if 0
    for (auto & s : SCALARTYPE_NAME) {
        if (s.t == t) {
            return &s;
        }
    }
#else
    const auto end = SCALARTYPE_NAME + A_ARRAY_SIZE(SCALARTYPE_NAME);
    const auto p = std::lower_bound(SCALARTYPE_NAME, end, t);
    if ((p != end) && (p->t == t)) {
        return p;
    }
#endif
    SDL_ASSERT(0);
    return nullptr;
}

} // namespace

// convert to number of seconds that have elapsed since 00:00:00 UTC, 1 January 1970
size_t datetime_t::get_unix_time(datetime_t const & src)
{
    SDL_ASSERT(!src.is_null());
    SDL_ASSERT(src.is_valid());

    size_t result = (size_t(src.d) - u_date_diff) * day_to_sec<1>::value;
    result += size_t(src.t) / 300;
    return result;
}

const char * obj_code::get_name(type const t)
{
    static_assert(A_ARRAY_SIZE(OBJ_CODE_NAME) == int(obj_code::type::_end), "");
    SDL_ASSERT(t != obj_code::type::_end);
    return OBJ_CODE_NAME[int(t)].name;
}

const char * obj_code::get_name(obj_code const d)
{
    type t = obj_code_type(d); // linear search
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
    struct tm const * const ptm = ::gmtime(&temp);
    SDL_ASSERT(ptm);
    if (ptm) {
        auto & tt = *ptm;
        result.d = u_date_diff + int32(val / day_to_sec<1>::value);
        result.t = tt.tm_sec;
        result.t += tt.tm_min * min_to_sec<1>::value;
        result.t += tt.tm_hour * hour_to_sec<1>::value;
        result.t *= 300;
    }
    return result;
}

nchar_t const * reverse_find(
    nchar_t const * const begin,
    nchar_t const * const end,
    nchar_t const * const buf,
    size_t const buf_size)
{
    SDL_ASSERT(buf_size);
    SDL_ASSERT(begin <= end);
    const size_t n = end - begin;
    if (n >= buf_size) {
        nchar_t const * p = end - buf_size;
        for (; begin <= p; --p) {
            if (!::memcmp(p, buf, sizeof(buf[0]) * buf_size)) {
                return p;
            }
        }
    }
    return nullptr;
}

scalartype::operator type() const
{
    scalartype::type const t = static_cast<type>(_32);
    if (find_scalartype(t)) {
        return t;
    }
    return scalartype::t_none;
}

const char * scalartype::get_name(type const t)
{
    if (auto s = find_scalartype(t)) {
        return s->name;
    }
    return "";
}

bool scalartype::is_fixed(type const t)
{
    if (auto s = find_scalartype(t)) {
        return s->fixed;
    }
    return false;
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

                    SDL_ASSERT(IS_LITTLE_ENDIAN);
                    static_assert(sizeof(uint8) == 1, "");
                    static_assert(sizeof(int16) == 2, "");
                    static_assert(sizeof(uint16) == 2, "");
                    static_assert(sizeof(int32) == 4, "");
                    static_assert(sizeof(uint32) == 4, "");
                    static_assert(sizeof(int64) == 8, "");
                    static_assert(sizeof(uint64) == 8, "");
                    static_assert(sizeof(nchar_t) == 2, "");

                    A_STATIC_ASSERT_IS_POD(guid_t);
                    A_STATIC_ASSERT_IS_POD(nchar_t);
                    A_STATIC_ASSERT_IS_POD(datetime_t);

                    static_assert(sizeof(pageType) == 1, "");
                    static_assert(sizeof(pageFileID) == 6, "");
                    static_assert(sizeof(recordID) == 8, "");
                    static_assert(sizeof(pageLSN) == 10, "");
                    static_assert(sizeof(pageXdesID) == 6, "");
                    static_assert(sizeof(guid_t) == 16, "");
                    static_assert(sizeof(nchar_t) == 2, "");
                    static_assert(sizeof(datetime_t) == 8, "");
                    static_assert(sizeof(bitmask8) == 1, "");

                    A_STATIC_ASSERT_IS_POD(auid_t);
                    static_assert(offsetof(auid_t, d.id) == 0x02, "");
                    static_assert(offsetof(auid_t, d.hi) == 0x06, "");
                    static_assert(sizeof(auid_t) == 8, "");

                    SDL_ASSERT(nchar_t{0x0030} == nchar_t{0x0030});
                    SDL_ASSERT(nchar_t{0x0030} != nchar_t{0});
                    {
                        const nchar_t test1[4] = { {0x006F}, {0x006E}, {0x0030}, {0x0030} };
                        const nchar_t test2[4] = { {0x0074}, {0x0069}, {0x006F}, {0x006E} };
                        const nchar_t nzero[2] = { {0x0030}, {0x0030} };
                        SDL_ASSERT(reverse_find({ test1, test1 + count_of(test1) }, nzero) == test1 + 2);
                        SDL_ASSERT(reverse_find({ test2, test2 + count_of(test2) }, nzero) == nullptr);
                    }
                    {
                        for (int i = 0; i < int(obj_code::type::_end); ++i) {
                            const obj_code::type t = obj_code::type(i);
                            auto s1 = obj_code::get_name(t);
                            auto s2 = obj_code::get_name(obj_code::get_code(t));
                            SDL_ASSERT(s1 == s2);
                        }
                    }
                    A_STATIC_ASSERT_IS_POD(obj_code);
                    A_STATIC_ASSERT_IS_POD(schobj_id);
                    A_STATIC_ASSERT_IS_POD(scalarlen);
                    A_STATIC_ASSERT_IS_POD(scalartype);

                    static_assert(sizeof(obj_code) == 2, "");
                    static_assert(sizeof(schobj_id) == 4, "");
                    static_assert(sizeof(scalarlen) == 2, "");
                    static_assert(sizeof(scalartype) == 4, "");

                    A_STATIC_ASSERT_IS_POD(pfs_byte);
                    static_assert(sizeof(pfs_byte) == 1, "");
                    static_assert(sizeof(pfs_byte::bitfields) == 1, "");
                    {
                        pfs_byte b{};
                        b.set_fullness(pfs_full::PCT_FULL_100);
                        SDL_ASSERT(b.fullness() == pfs_full::PCT_FULL_100);
                    }
                    static_assert(pageType::size == 21, "");
                    static_assert(dataType::size == 4, "");
                    SDL_ASSERT(std::is_sorted(std::begin(SCALARTYPE_NAME), std::end(SCALARTYPE_NAME)));
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG

#if 0
/*
 *----------------------------------------------------------------------
 *
 * strstr --
 *
 *  Locate the first instance of a substring in a string.
 *
 * Results:
 *  If string contains substring, the return value is the
 *  location of the first matching instance of substring
 *  in string.  If string doesn't contain substring, the
 *  return value is 0.  Matching is done on an exact
 *  character-for-character basis with no wildcards or special
 *  characters.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */
char *
strstr(string, substring)
    register char *string;  /* String to search. */
    char *substring;        /* Substring to try to find in string. */
{
    register char *a, *b;

    /* First scan quickly through the two strings looking for a
     * single-character match.  When it's found, then compare the
     * rest of the substring.
     */

    b = substring;
    if (*b == 0) {
        return string;
    }
    for ( ; *string != 0; string += 1) {
        if (*string != *b) {
            continue;
        }
        a = string;
        while (1) {
            if (*b == 0) {
                return string;
            }
            if (*a++ != *b++) {
                break;
            }
        }
        b = substring;
    }
    return (char *) 0;
}
#endif
