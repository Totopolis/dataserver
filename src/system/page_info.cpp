// page_info.cpp
//
#include "common/common.h"
#include "page_info.h"
#include <time.h>       /* time_t, struct tm, time, localtime, strftime */

namespace sdl { namespace db { namespace {

std::string type_raw_bytes(void const * _buf, size_t const buf_size,
    size_t const separate_byte = 0)
{
    SDL_ASSERT(buf_size);
    SDL_ASSERT(separate_byte < buf_size);
    char const * buf = (char const *)_buf;
    char xbuf[128] = {};
    std::stringstream ss;
    for (size_t i = 0; i < buf_size; ++i) {
        if (separate_byte && (separate_byte == i))
            ss << " ";
        auto n = static_cast<uint8>(buf[i]); // signed to unsigned
        ss << format_s(xbuf, "%02X", int(n));
    }
    return ss.str();
}

std::string type_raw_buf(void const * _buf, size_t const buf_size, bool const show_address = false)
{
    char const * buf = (char const *)_buf;
    SDL_ASSERT(buf_size);
    char xbuf[128] = {};
    std::stringstream ss;
    for (size_t i = 0; i < buf_size; ++i) {
        if (show_address) {
            if (!(i % 20)) {
                ss << "\n";
                ss << format_s(xbuf, "%016X: ", i);
            }
            if (!(i % 4)) {
                ss << " ";
            }
        }
        auto n = static_cast<uint8>(buf[i]);
        ss << format_s(xbuf, "%02X", int(n));
    }
    ss << std::endl;
    return ss.str();
}

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

#if 0
struct obj_sys_name
{
    const char * name;
    uint32 object_id;
    obj_sys_name(const char * n, int i): name(n), object_id(i) {}
};

const obj_sys_name OBJ_SYS_NAME[] = {
{ "sysrscols",                  3 },
{ "sysrowsets",                 5 },
{ "sysclones",                  6 },
{ "sysallocunits",              7 },
{ "sysfiles1",                  8 },
{ "sysseobjvalues",             9 },
{ "sysmatrixages",              16 },
{ "syspriorities",              17 },
{ "sysdbfrag",                  18 },
{ "sysfgfrag",                  19 },
{ "sysdbfiles",                 20 },
{ "syspru",                     21 },
{ "sysbrickfiles",              22 },
{ "sysphfg",                    23 },
{ "sysprufiles",                24 },
{ "sysftinds",                  25 },
{ "sysowners",                  27 },
{ "sysdbreg",                   28 },
{ "sysprivs",                   29 },
{ "sysschobjs",                 34 },
{ "syscsrowgroups",             35 },
{ "sysextsources",              36 },
{ "sysexttables",               37 },
{ "sysextfileformats",          38 },
{ "syslogshippers",             39 },
{ "syscolpars",                 41 },
{ "sysxlgns",                   42 },
{ "sysxsrvs",                   43 },
{ "sysnsobjs",                  44 },
{ "sysusermsgs",                45 },
{ "syscerts",                   46 },
{ "sysrmtlgns",                 47 },
{ "syslnklgns",                 48 },
{ "sysxprops",                  49 },
{ "sysscalartypes",             50 },
{ "systypedsubobjs",            51 },
{ "sysidxstats",                54 },
{ "sysiscols",                  55 },
{ "sysendpts",                  56 },
{ "syswebmethods",              57 },
{ "sysbinobjs",                 58 },
{ "sysaudacts",                 59 },
{ "sysobjvalues",               60 },
{ "sysmatrixconfig",            61 },
{ "syscscolsegments",           62 },
{ "syscsdictionaries",          63 },
{ "sysclsobjs",                 64 },
{ "sysrowsetrefs",              65 },
{ "sysremsvcbinds",             67 },
{ "sysxmitqueue",               68 },
{ "sysrts",                     69 },
{ "sysmatrixbricks",            70 },
{ "sysconvgroup",               71 },
{ "sysdesend",                  72 },
{ "sysdercv",                   73 },
{ "syssingleobjrefs",           74 },
{ "sysmultiobjrefs",            75 },
{ "sysmatrixmanagers",          77 },
{ "sysguidrefs",                78 },
{ "sysfoqueues",                79 },
{ "syschildinsts",              80 },
{ "sysextendedrecoveryforks",   81 },
{ "syscompfragments",           82 },
{ "sysmatrixageforget",         83 },
{ "sysftsemanticsdb",           84 },
{ "sysftstops",                 85 },
{ "sysftproperties",            86 },
{ "sysxmitbody",                87 },
{ "sysfos",                     89 },
{ "sysqnames",                  90 },
{ "sysxmlcomponent",            91 },
{ "sysxmlfacet",                92 },
{ "sysxmlplacement",            93 },
{ "sysobjkeycrypts",            94 },
{ "sysasymkeys",                95 },
{ "syssqlguides",               96 },
{ "sysbinsubobjs",              97 },
{ "syssoftobjrefs",             98 },
};
#endif

} // namespace

const char * to_string::type_name(pageType const t)
{
    switch (t.val) {
    case pageType::null: return "null";
    case pageType::data: return "data";
    case pageType::index: return "index";
    case pageType::textmix: return "textmix";
    case pageType::texttree: return "texttree";
    case pageType::sort: return "sort";
    case pageType::GAM: return "GAM";
    case pageType::SGAM: return "SGAM";
    case pageType::IAM: return "IAM";
    case pageType::PFS: return "PFS";
    case pageType::boot: return "boot";
    case pageType::fileheader: return "fileheader";
    case pageType::diffmap: return "diffmap";
    case pageType::MLmap: return "MLmap";
    case pageType::deallocated: return "deallocated";
    case pageType::temporary: return "temporary";
    case pageType::preallocated: return "preallocated";
    default:
        return ""; // unknown type
    }
}

const char * to_string::code_name(obj_code const & d)
{
    static_assert(A_ARRAY_SIZE(OBJ_CODE_NAME) == 26, "");
    for (auto & it : OBJ_CODE_NAME) {
        if (it.code.u == d.u)
            return it.name;
    }
    SDL_WARNING(0);
    return "";
}

#if 0
const char * to_string::sys_name(auid_t const & d)
{
    for (auto & it : OBJ_SYS_NAME) {
        A_STATIC_SAME_TYPE(it.object_id, d.d.id);
        if (it.object_id == d.d.id)
            return it.name;
    }
    SDL_WARNING(0);
    return "";
}
#endif

std::string to_string::type(pageType const t)
{
    char buf[128];
    return std::string(format_s(buf, "%s(%d)", type_name(t), int(t.val)));
}

std::string to_string::type_raw(char const * buf, size_t const buf_size)
{
    return type_raw_buf(buf, buf_size, true);
}

std::string to_string::dump_mem(void const * buf, size_t const buf_size)
{
    return type_raw_bytes(buf, buf_size);
}

std::string to_string::type(uint8 value)
{
    char buf[64];
    return std::string(format_s(buf, "%d", int(value)));
}

std::string to_string::type(guid_t const & g)
{
    char buf[128] = {};
    std::stringstream ss;
    ss << format_s(buf, "%x", uint32(g.a)) << "-";
    ss << format_s(buf, "%x", uint32(g.b)) << "-";
    ss << format_s(buf, "%x", uint32(g.c)) << "-";
    ss << format_s(buf, "%x", uint32(g.d));
    ss << format_s(buf, "%x", uint32(g.e)) << "-";;
    ss << format_s(buf, "%x", uint32(g.f));
    ss << format_s(buf, "%x", uint32(g.g));
    ss << format_s(buf, "%x", uint32(g.h));
    ss << format_s(buf, "%x", uint32(g.i));
    ss << format_s(buf, "%x", uint32(g.j));
    ss << format_s(buf, "%x", uint32(g.k));
    return ss.str();
}

std::string to_string::type(pageLSN const & d)
{
    std::stringstream ss;
    ss << d.lsn1 << ":" << d.lsn2 << ":" << d.lsn3;
    return ss.str();
}

std::string to_string::type(pageFileID const & d)
{
    std::stringstream ss;
    ss << d.fileId << ":" << d.pageId;
    ss << " (";
    ss << type_raw_bytes(&d, sizeof(d));
    ss << ")";
    return ss.str();
}

std::string to_string::type(pageXdesID const & d)
{
    std::stringstream ss;
    ss << d.id1 << ":" << d.id2;
    return ss.str();
}

std::string to_string::type(nchar_t const * p, const size_t buf_size)
{
    static_assert(sizeof(*p) == 2, "");
    auto const end = p + buf_size;
    std::string s;
    s.reserve(buf_size);
    for (; p != end; ++p) {
        char c = *reinterpret_cast<char const *>(p);
        if (c)
            s += c;
        else
            break;
    }
    return s;
}

std::string to_string::type(nchar_range const & p)
{
    enum { dump_name = 1 };
    if (p.first != p.second) {
        SDL_ASSERT(p.first < p.second);
        char buf[128];
        const size_t n = p.second - p.first;
        std::string s = to_string::type(p.first, n);
        s += format_s(buf, "\nnchar = %d", int(n));
        s += format_s(buf, "\nbytes = %d", int(n * sizeof(nchar_t)));
        if (dump_name) {
            s += "\n";
            s += type_raw_buf(p.first, n * sizeof(nchar_t), true);
        }
        return s;
    }
    return std::string();
}

std::string to_string::type(datetime_t const & src)
{
    if (src.is_valid()) {
        time_t temp = static_cast<time_t>(src.get_unix_time());
        struct tm * ptm = ::gmtime(&temp);
        if (ptm) {
            char tmbuf[128];
            strftime(tmbuf, sizeof(tmbuf), "%Y-%m-%d %H:%M:%S", ptm);
            tmbuf[count_of(tmbuf) - 1] = 0;
            std::string s(tmbuf);
            s += " (";
            s += type_raw_bytes(&src, sizeof(src));
            s += ")";
            return s;
        }
    }
    SDL_ASSERT(0);
    return std::string();
}

std::string to_string::type(slot_array const & slot)
{
    enum { print_line = 0 };
    enum { print_dump = 1 };

    std::stringstream ss;
    ss << "slot_array[" << slot.size() << "] = ";
    if (print_line) {
        for (size_t j = 0; j < slot.size(); ++j) {
            if (j) { ss << " "; }
            ss << slot[j];
        }
    }
    else {
        for (size_t j = 0; j < slot.size(); ++j) {
            auto const val = slot[j];
            ss << "\n[" << j << "] = 0x"
                << std::hex << val << " ("
                << std::dec << val << ")";
        }
    }
    if (print_dump) {
        auto p1 = slot.rbegin();
        auto p2 = slot.rend();
        SDL_ASSERT((p2 - p1) == slot.size());
        ss << "\n\nDump slot_array:\n";
        ss << type_raw_buf(p1, (p2 - p1) * sizeof(*p1), true);
    }
    return ss.str();
}

std::string to_string::type(null_bitmap const & data)
{
    std::stringstream ss;
    ss << "\nnull_bitmap = " << data.size();
    ss << " Length (bytes) = " << (data.end() - data.begin());
    for (size_t i = 0; i < data.size(); ++i) {
        ss << "\n[" << i << "] = " << data[i];        
    }
    auto s = ss.str();
    s += "\n(";
    size_t n = data.end() - data.begin();
    s += type_raw_bytes(data.begin(), n, sizeof(null_bitmap::column_num));
    s += ")";
    return s;
}

std::string to_string::type(variable_array const & data)
{
    std::stringstream ss;
    ss << "\nvariable_array = " << data.size();
    ss << " Length (bytes) = " << (data.end() - data.begin());
    for (size_t i = 0; i < data.size(); ++i) {
        auto const & d = data[i];
        A_STATIC_CHECK_TYPE(uint16, d.first);
        A_STATIC_CHECK_TYPE(bool, d.second);
        ss << "\n[" << i << "] = " 
            << d.first << " (" << std::hex
            << d.first << ")" << std::dec; 
        if (d.second) {
            ss << " COMPLEX Offset = " << variable_array::offset(d);
            auto const & pp = data.row_overflow(i);
            if (pp.first && pp.second) {
                ss << " ROW_OVERFLOW[" << pp.second << "] = ";
                A_STATIC_CHECK_TYPE(overflow_page const *, pp.first);
                A_STATIC_CHECK_TYPE(size_t, pp.second);
                for (size_t i = 0; i < pp.second; ++i) {
                    if (i) ss << " ";
                    ss << to_string::type(pp.first[i]);
                }
            }
            else {
                SDL_TRACE(variable_array::offset(d));
                SDL_ASSERT(false);
                ss << " ROW_OVERFLOW = ?";
            }
        }
    }
    auto s = ss.str();
    s += "\n(";
    const size_t n = data.end() - data.begin();
    s += type_raw_bytes(data.begin(), n, sizeof(variable_array::column_num));
    s += ")";
    return s;
}

std::string to_string::type(auid_t const & id)
{
    std::stringstream ss;
    ss << int(id.d.lo) << ":"
        << int(id.d.id) << ":"
        << int(id.d.hi)
        << " ("
        << "0x" << std::uppercase << std::hex
        << id._64
        << ")";
    return ss.str();
}

std::string to_string::type(bitmask8 const & b)
{
    char buf[64];
    std::string s(format_s(buf, "%d (", int(b.byte)));
    auto val = b.byte;
    for (size_t i = 0; i < 8; ++i) {
        s += (val & 0x80) ? '1' : '0';
        val <<= 1;
    }
    s += ")";
    return s;
}

std::string to_string::type_nchar(
    row_head const & head,
    size_t const col_index)
{
    if (!(head.has_null() && head.has_variable())) {
        return "[NULL]";
    }
    if (!null_bitmap(&head)[col_index]) {
        variable_array const var(&head);
        if (!var.empty()) {
            auto const d = var.var_data(col_index);
            const size_t n = (d.second - d.first) / sizeof(nchar_t); // length in nchar_t
            if (n) {
                auto p = reinterpret_cast<nchar_t const *>(d.first);
                auto s = db::to_string::type(nchar_range(p, p + n));
                return s;
            }
            SDL_ASSERT(0);
        }
    }
    return "";
}

std::string to_string::type(obj_code const & d)
{
    std::stringstream ss;
    ss << d.u << " \"" << d.c[0] << d.c[1] << "\"";
    ss << " " << to_string::code_name(d);
    return ss.str();
}

std::string to_string::type(overflow_page const & d)
{
    enum { dump_raw = 0 };
    std::stringstream ss;
    ss << d.id.fileId << ":"
        << d.id.pageId << ":"
        << d.slot;
    if (dump_raw) {
        ss << " (";
        ss << type_raw_bytes(&d, sizeof(d), sizeof(d.meta));
        ss << ")";
    }
    return ss.str();
}

//-----------------------------------------------------------------

std::string page_info::type_meta(page_head const & p)
{
    std::stringstream ss;
    impl::processor<page_head_meta::type_list>::print(ss, &p);
    return ss.str();
}

std::string page_info::type_raw(page_head const & p)
{
    return to_string::type_raw(p.raw);
}

std::string page_info::type_meta(row_head const & h)
{
    std::stringstream ss;
    impl::processor<row_head_meta::type_list>::print(ss, &h);
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
                    datetime_t d1 = {};
                    d1.d = 42003; // = SELECT DATEDIFF(d, '19000101', '20150101');
                    d1.t = 300;
                    //SDL_ASSERT(to_string::type(d1) == "2015-01-01 00:00:01");
                    SDL_ASSERT(to_string::type(d1) == "2015-01-01 00:00:01 (2C01000013A40000)");
                    auto const ut = datetime_t::get_unix_time(d1);
                    const datetime_t d2 = datetime_t::set_unix_time(ut);
                    SDL_ASSERT(d1.d == d2.d);
                    SDL_ASSERT(d1.t == d2.t);
                    {
                        static_assert(binary<1>::value == 1, "");
                        static_assert(binary<11>::value == 3, "");
                        static_assert(binary<101>::value == 5, "");
                        static_assert(binary<111>::value == 7, "");
                        static_assert(binary<1001>::value == 9, "");
                        static_assert(binary<10000000>::value == 128, "");
                        static_assert(binary<11111111>::value == 255, "");
                        SDL_ASSERT(to_string::type(bitmask8{ binary<11111111>::value }) == "255 (11111111)");
                        SDL_ASSERT(to_string::type(bitmask8{ binary<10101010>::value }) == "170 (10101010)");
                        SDL_ASSERT(to_string::type(bitmask8{ binary< 1010101>::value }) ==  "85 (01010101)");
                    }
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG


