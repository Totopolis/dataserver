// page_info.cpp
//
#include "common/common.h"
#include "page_info.h"
#include "spatial/transform.h"
#include <time.h>       /* time_t, struct tm, time, localtime, strftime */
#include <iomanip>      // for std::setprecision

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

template<size_t buf_size> inline
std::string type_raw_bytes(char const(&buf)[buf_size]) {
    return type_raw_bytes(buf, buf_size);
}

std::string type_raw_buf(void const * _buf, size_t const buf_size, bool const show_address)
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

static std::atomic<int> to_string_precision; // = 0

} // namespace

int to_string::precision()
{
    return to_string_precision;
}

void to_string::precision(int value)
{
    to_string_precision = a_max(0, value);
}

const char * to_string::type_name(pageType::type const t)
{
    switch (t) {
    case pageType::type::null: return "null";
    case pageType::type::data: return "data";
    case pageType::type::index: return "index";
    case pageType::type::textmix: return "textmix";
    case pageType::type::texttree: return "texttree";
    case pageType::type::sort: return "sort";
    case pageType::type::GAM: return "GAM";
    case pageType::type::SGAM: return "SGAM";
    case pageType::type::IAM: return "IAM";
    case pageType::type::PFS: return "PFS";
    case pageType::type::boot: return "boot";
    case pageType::type::fileheader: return "fileheader";
    case pageType::type::diffmap: return "diffmap";
    case pageType::type::MLmap: return "MLmap";
    case pageType::type::deallocated: return "deallocated";
    case pageType::type::temporary: return "temporary";
    case pageType::type::preallocated: return "preallocated";
    default:
        SDL_ASSERT(0);
        return ""; // unknown type
    }
}

const char * to_string::type_name(pageType const t)
{
    return type_name(static_cast<pageType::type>(t));
}

const char * to_string::type_name(dataType::type const t)
{
    switch (t) {
    case dataType::type::null: return "null";
    case dataType::type::IN_ROW_DATA: return "IN_ROW_DATA";
    case dataType::type::LOB_DATA: return "LOB_DATA";
    case dataType::type::ROW_OVERFLOW_DATA: return "ROW_OVERFLOW_DATA";
    default:
        SDL_ASSERT(0);
        return ""; // unknown type
    }
}

const char * to_string::type_name(dataType const t)
{
    return type_name(static_cast<dataType::type>(t));
}

const char * to_string::type_name(recordType const t) 
{
    switch (t) {
    case recordType::primary_record:    return "primary_record"; 
    case recordType::forwarded_record:  return "forwarded_record"; 
    case recordType::forwarding_record: return "forwarding_record"; 
    case recordType::index_record:      return "index_record"; 
    case recordType::blob_fragment:     return "blob_fragment"; 
    case recordType::ghost_index:       return "ghost_index"; 
    case recordType::ghost_data:        return "ghost_data"; 
    case recordType::ghost_version:     return "ghost_version"; 
    default:
        SDL_ASSERT(0);
        return ""; // unknown type
    }
}

const char * to_string::type_name(pfs_full const t)
{
    switch (t) {
    case pfs_full::PCT_FULL_0 : return "PCT_FULL_0";
    case pfs_full::PCT_FULL_50 : return "PCT_FULL_50";
    case pfs_full::PCT_FULL_80 : return "PCT_FULL_80";
    case pfs_full::PCT_FULL_95 : return "PCT_FULL_95";
    case pfs_full::PCT_FULL_100 : return "PCT_FULL_100";
    default:
        SDL_ASSERT(0);
        return ""; // unknown type
    }
}

const char * to_string::type_name(sortorder const t)
{
    switch (t) {
    case sortorder::NONE : return "NONE";
    case sortorder::ASC  : return "ASC";
    case sortorder::DESC : return "DESC";
    default:
        SDL_ASSERT(0);
        return ""; // unknown type
    }
}

const char * to_string::type_name(spatial_type const t)
{
    switch (t) {
    case spatial_type::point            : return "point";
    case spatial_type::linestring       : return "linestring";
    case spatial_type::polygon          : return "polygon";
    case spatial_type::linesegment      : return "linesegment";
    case spatial_type::multilinestring  : return "multilinestring";
    case spatial_type::multipolygon     : return "multipolygon";
    //multipoint,
    default:
        SDL_ASSERT(t == spatial_type::null);
        return ""; // unknown type
    }
}

const char * to_string::obj_name(obj_code const d)
{
    return obj_code::get_name(d);
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
    return std::string(format_s(buf, "%s(%d)", type_name(t), int(t.value)));
}

std::string to_string::type(dataType const t)
{
    char buf[128];
    return std::string(format_s(buf, "%s(%d)", type_name(t), int(t.value)));
}

std::string to_string::type(idxtype const t)
{
    char buf[128];
    return std::string(format_s(buf, "%s(%d)", idxtype::get_name(t), int(t._8)));
}

std::string to_string::type(idxstatus const t)
{
    enum { show_binary = 0 };
    std::stringstream ss;
    ss << t._32 << " (";
    if (show_binary) {
        uint32 val = t._32;
        for (size_t i = 0; i < 32; ++i) {
            if (i && !(i % 8))
                ss << " ";
            ss << ((val & 0x80000000) ? "1" : "0");
            val <<= 1;
        }
    }
    else {
        ss << "0x" << std::hex << t._32 << std::dec;
    }
    ss << ")";
    if (t.IsUnique())           { ss << " IsUnique"; }
    if (t.IsPrimaryKey())       { ss << " IsPrimaryKey"; }
    if (t.IsUniqueConstraint()) { ss << " IsUniqueConstraint"; }
    if (t.IsPadded())           { ss << " IsPadded"; }
    if (t.IsDisabled())         { ss << " IsDisabled"; }
    if (t.IsHypothetical())     { ss << " IsHypothetical"; }
    if (t.HasFilter())          { ss << " HasFilter"; }
    if (t.AllowRowLocks())      { ss << " AllowRowLocks"; }
    if (t.AllowPageLocks())     { ss << " AllowPageLocks"; }
    return ss.str();
}

std::string to_string::type(scalartype const t)
{
    char buf[128];
    return std::string(format_s(buf, "%s(%d)", scalartype::get_name(t), int(t._32)));
}

std::string to_string::type_raw(char const * buf, size_t const buf_size)
{
    return type_raw_buf(buf, buf_size, true);
}

std::string to_string::type_raw(char const * buf, size_t const buf_size, type_format const f)
{
    return type_raw_buf(buf, buf_size, type_format::more == f);
}

std::string to_string::dump_mem(void const * buf, size_t const buf_size)
{
    if (buf_size)
        return type_raw_bytes(buf, buf_size);
    return{};
}

std::string to_string::dump_mem(vector_mem_range_t const & data)
{
    std::string s;
    for (auto & m : data) {
        s += dump_mem(m);
    }
    return s;
}

std::string to_string::dump_mem(var_mem const & v)
{
    return dump_mem(v.data());
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

std::string to_string::type(spatial_cell const & d, type_format const f)
{
    enum { trace_xy = 1 };
    char buf[128] = {};
    std::stringstream ss;
    for (size_t i = 0; i < spatial_cell::size; ++i) {
        ss << format_s(buf, "%d", uint32(d.data.id.cell[i])) << "-"; 
    }
    ss << format_s(buf, "%d", uint32(d.data.depth)); 
    if (f == type_format::more) {
        ss << " (" << type_raw_bytes(d.raw) << ")";
        if (trace_xy) {
            auto const xy = transform::d2xy(d[0]);
            auto const pos = transform::cell_point(d);
            auto const sp = transform::spatial(d);
            ss << std::setprecision(9);
            ss << " POINT(" << sp.longitude << " " << sp.latitude << ")";
            ss << " (X = " << xy.X << ", Y = " << xy.Y << ")";
            ss << " (" << pos.X << "," << pos.Y << ")";
        }
    }
    return ss.str();
}

std::string to_string::type(spatial_cell const & d)
{
    return to_string::type(d, type_format::more);
}

std::string to_string::type(spatial_point const & d)
{
    std::stringstream ss;
    ss << "(longitude = " << d.longitude << ", latitude = " << d.latitude << ")";
    return ss.str();
}

namespace {

#pragma pack(push, 1) 

struct guid_le
{
    struct s1 {
        uint8 e;
        uint8 d;
    };
    struct s2 {
        uint8 k; // 6
        uint8 j; // 5
        uint8 i; // 4
        uint8 h; // 3
        uint8 g; // 2
        uint8 f; // 1
    };
    union {
        s1 s;
        uint16 _16;
    } d;
    union {
        s2 s;
        uint64 _64;
    } f;
};
#pragma pack(pop)

} // namespace

guid_t to_string::parse_guid(std::stringstream & ss)
{
    db::guid_t k{};
    char c = 0;
    guid_le g;
    ss << std::hex;
    ss >> k.a; ss >> c;
    ss >> k.b; ss >> c;
    ss >> k.c; ss >> c; 
    ss >> g.d._16; ss >> c; 
    ss >> g.f._64;
    k.d = g.d.s.d;
    k.e = g.d.s.e;
    k.f = g.f.s.f;
    k.g = g.f.s.g;
    k.h = g.f.s.h;
    k.i = g.f.s.i;
    k.j = g.f.s.j;
    k.k = g.f.s.k;
    return k;
}

guid_t to_string::parse_guid(std::string const & s)
{
    if (!s.empty()) {
        std::stringstream ss(s);
        return parse_guid(ss);
    }
    SDL_ASSERT(0);
    return{};
}

std::string to_string::type(pageLSN const & d)
{
    std::stringstream ss;
    ss << d.lsn1 << ":" << d.lsn2 << ":" << d.lsn3;
    return ss.str();
}

std::string to_string::type(pageFileID const & d, type_format const f)
{
    std::stringstream ss;
    ss << d.fileId << ":" << d.pageId;
    if (f == type_format::more) {
        ss << " (";
        ss << type_raw_bytes(&d, sizeof(d));
        ss << ")";
    }
    return ss.str();
}

std::string to_string::type(pageFileID const & d)
{
    return to_string::type(d, type_format::more);
}

std::string to_string::type(pageFileID const * const pages, const size_t page_size)
{
    char buf[128] = {};
    std::string s;
    for (size_t i = 0; i < page_size; ++i) {
        s += format_s(buf, "\n[%d] = ", int(i));
        s += to_string::type(pages[i]);
    }
    return s;
}

std::string to_string::type(pageXdesID const & d)
{
    std::stringstream ss;
    ss << d.id1 << ":" << d.id2;
    return ss.str();
}

std::string to_string::type(char const * p, const size_t buf_size)
{
    return std::string(p, p + buf_size);
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

std::string to_string::type(nchar_range const & p, const type_format f)
{
    enum { dump_name = 1 };
    if (p.first != p.second) {
        SDL_ASSERT(p.first < p.second);
        const size_t n = p.second - p.first;
        std::string s = to_string::type(p.first, n);
        if (f == type_format::more) {
            char buf[128];
            s += format_s(buf, "\nnchar = %d", int(n));
            s += format_s(buf, "\nbytes = %d", int(n * sizeof(nchar_t)));
            if (dump_name) {
                s += "\n";
                s += type_raw_buf(p.first, n * sizeof(nchar_t), true);
            }
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

std::string to_string::type(smalldatetime_t const d)
{
    std::stringstream ss;
    ss << "[day:" << d.day << " min:" << d.min << "]";
    return ss.str();
}

std::string to_string::type(numeric9 const d)
{
    SDL_ASSERT(d._8 == 1);
    std::stringstream ss;
    ss << d._64;
    return ss.str();
}

std::string to_string::type(slot_array const & slot)
{
    enum { print_line = 0 };
    enum { print_dump = 0 };

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
        SDL_ASSERT((size_t)(p2 - p1) == slot.size());
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
            auto const type = data.var_complextype(i);
            ss << " COMPLEX (" << complextype::get_name(type) 
                << ") Offset = " << variable_array::offset(d);
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
        << ")"
        << " ("
        << std::uppercase << std::dec
        << id._64
        << ")";
    return ss.str();
}

std::string to_string::type(bitmask8 const b)
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

std::string to_string::type(pfs_byte const f)
{
    enum { bitmask = 0 };
    auto const b = f.b;
    std::stringstream ss;
    ss << (b.allocated ? "ALLOCATED" : "NOT ALLOCATED");
    ss << " " << type_name(f.fullness());
    if (b.iam) {
        ss << " IAM Page";
    }
    if (b.mixed) {
        ss << " Mixed Ext";
    }
    if (b.ghost) {
        ss << " Ghost";
    }
    if (bitmask) {
        ss << " bitmask = " << to_string::type(bitmask8{ f.byte });
    }
    return ss.str();
}

std::string to_string::type_nchar(
    row_head const & head,
    size_t const col_index,
    const type_format f)
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
                return to_string::type(nchar_range(p, p + n), f);
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
    ss << " " << to_string::obj_name(d);
    return ss.str();
}

std::string to_string::type(overflow_page const & d)
{
    enum { dump_raw = 0 };
    std::stringstream ss;
    ss << d.row.id.fileId << ":"
        << d.row.id.pageId << ":"
        << d.row.slot;
    if (dump_raw) {
        ss << " (";
        ss << type_raw_bytes(&d, sizeof(d));
        ss << ")";
    }
    return ss.str();
}

std::string to_string::type(recordID const & d)
{
    enum { dump_raw = 0 };
    std::stringstream ss;
    ss << d.id.fileId << ":"
        << d.id.pageId << ":"
        << d.slot;
    if (dump_raw) {
        ss << " (";
        ss << type_raw_bytes(&d, sizeof(d));
        ss << ")";
    }
    return ss.str();
}

std::string to_string::type(text_pointer const & d)
{
    return to_string::type(d.row);
}

std::string to_string::make_text(vector_mem_range_t const & data)
{
    std::string s;
    s.reserve(mem_size_n(data));
    for (auto & m : data) {
        s.append(m.first, m.second);
    }
    return s;
}

std::string to_string::make_ntext(vector_mem_range_t const & data)
{
    std::string s;
    for (auto & m : data) {
        s += to_string::type(make_nchar_checked(m));
    }
    return s;
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

//-----------------------------------------------------------------

std::string to_string_with_head::type(row_head const & h)
{
    std::stringstream ss;
    ss  << "\nstatusA = " << type(h.data.statusA) << " " 
        << type_name(h.get_type())
        << "\nstatusB = " << type(h.data.statusB)
        << "\nfixedlen = " << h.data.fixedlen
        << std::endl;
    return ss.str();
}

//-----------------------------------------------------------------

static const int geo_precision = 9;

std::string to_string::type(geo_point const & p)
{
    std::stringstream ss;
    ss << std::setprecision(geo_precision)
        << "POINT ("         
        << p.data.point.longitude << " "
        << p.data.point.latitude << ")";
    return ss.str();
}

std::string to_string::type(geo_linesegment const & data)
{
    std::stringstream ss;
    ss << std::setprecision(geo_precision)
        << "LINESTRING ("         
        << data[0].longitude << " "
        << data[0].latitude << ", "
        << data[1].longitude << " "
        << data[1].latitude << ")";
    return ss.str();
}

namespace {

std::string type_geo_pointarray(geo_pointarray const & data, const char * title)
{
    std::stringstream ss;
    ss << std::setprecision(geo_precision);
    if (is_str_valid(title)) {
        ss << title << " ";
    }
    ss << "(";
    for (size_t i = 0; i < data.size(); ++i) {
        const auto & pt = data[i];
        if (i) {
            ss << ", ";
        }
        ss << pt.longitude << " " << pt.latitude;
    }
    ss << ")";
    return ss.str();
}

std::string type_geo_multi(geo_mem const & data, const char * const title)
{
    std::stringstream ss;
    ss << std::setprecision(geo_precision);
    ss << title << " (";
    const size_t numobj = data.numobj();
    SDL_ASSERT(numobj);
    for (size_t i = 0; i < numobj; ++i) {
        auto const pp = data.get_subobj(i);
        SDL_ASSERT(pp.size());
        if (i) {
            ss << ", ";
        }
        ss << "(";
        size_t count = 0;
        for (auto const & p : pp) {
            if (count++) {
                ss << ", ";
            }
            ss << p.longitude << " " << p.latitude;
        }
        ss << ")";
    }
    ss << ")";
    return ss.str();
}

} // namespace

std::string to_string::type(geo_pointarray const & data)
{
    return type_geo_pointarray(data, "");
}

std::string to_string::type(geo_mem const & data)
{
    switch (data.type()) {
    case spatial_type::point:           return type(*data.cast_point());
    case spatial_type::linestring:      return type_geo_pointarray(*data.cast_linestring(), "LINESTRING");
    case spatial_type::polygon:         return type_geo_pointarray(*data.cast_polygon(), "POLYGON");
    case spatial_type::linesegment:     return type(*data.cast_linesegment());
    case spatial_type::multilinestring: return type_geo_multi(data, "MULTILINESTRING");
    case spatial_type::multipolygon:    return type_geo_multi(data, "MULTIPOLYGON");
    default:
        SDL_ASSERT(0);
        break;
    }
    return{};
}

//-----------------------------------------------------------------

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
                    A_STATIC_ASSERT_IS_POD(guid_le);
                    const char * const g = "a0e315c1-c80c-4f09-8adc-040a0c74f18";
                    SDL_ASSERT(to_string::type(to_string::parse_guid(g)) == g);
                    static_assert(sizeof(guid_le) == 10, "");
                    if (0) {
                        const double value = 1.12345678;
                        auto const old = to_string::precision();
                        to_string::precision(4); SDL_TRACE(to_string::type(value));
                        to_string::precision(17); SDL_TRACE(to_string::type(value));
                        to_string::precision(old); SDL_TRACE(to_string::type(value));
                    }
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG


