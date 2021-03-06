// page_info.cpp
//
#include "dataserver/system/page_info.h"
#include "dataserver/system/type_utf.h" // to be tested
#include "dataserver/spatial/transform.h"
#include "dataserver/common/time_util.h"
#include "dataserver/common/format.h"
#include <iomanip>      // for std::setprecision

namespace sdl { namespace db { namespace {

//-------------------------------------------------------------------------

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

template<class T> inline 
std::string type_raw_bytes(T const & v) {
    return type_raw_bytes(&v, sizeof(v));
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

static std::atomic<int> to_string_precision(6);

} // namespace

int to_string::precision()
{
    return to_string_precision;
}

void to_string::precision(int value)
{
    to_string_precision = a_min_max<int>(value, 0, limits::double_max_digits10);
}

to_string::stringstream &
to_string::stringstream::operator << (double const value)
{
    if (int p = to_string_precision) {
        char buf[64];
        ss << format_double(buf, value, p);
    }
    else {
        ss << value;
    }
    return *this;
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
    //FIXME: spatial_type::multipoint
    default:
        SDL_ASSERT(t == spatial_type::null);
        return ""; // unknown type
    }
}

const char * to_string::type_name(geometry_types const t)
{
    switch (t) {
    case geometry_types::Point              : return "Point";
    case geometry_types::LineString         : return "LineString";
    case geometry_types::Polygon            : return "Polygon";
    case geometry_types::MultiPoint         : return "MultiPoint";
    case geometry_types::MultiLineString    : return "MultiLineString";
    case geometry_types::MultiPolygon       : return "MultiPolygon";
    case geometry_types::GeometryCollection : return "GeometryCollection";
    default:
        SDL_ASSERT(t == geometry_types::Unknown);
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
    if (buf_size) {
        std::string s("dump(");
        s += type_raw_bytes(buf, buf_size);
        s += ")";
        return s;
    }
    return{};
}

std::string to_string::dump_mem(vector_mem_range_t const & data)
{
    if (mem_empty(data)) {
        return{};
    }
    std::string s("dump(");
    for (auto & m : data) {
        s += type_raw_bytes(m);
    }
    s += ")";
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
            auto const pos = transform::cell2point(d);
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
    to_string::stringstream ss;
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
    if (src.is_null()) {
        return {};
    }
    const gregorian_t d = src.gregorian();
    const clocktime_t t = src.clocktime();
    char tmbuf[128];
    format_s(tmbuf, "%d-%02d-%02d %02d:%02d:%02d.%03d",
        d.year, d.month, d.day,
        t.hour, t.min, t.sec, t.milliseconds);
    return std::string(tmbuf);
}

std::string to_string::type(smalldatetime_t const d)
{
    std::stringstream ss;
    ss << "[day:" << d.day << " min:" << d.min << "]";
    return ss.str();
}

/*std::string to_string::type(numeric9 const d)
{
    SDL_ASSERT(d._8 == 1);
    std::stringstream ss;
    ss << d._64;
    return ss.str();
}*/

std::string to_string::type(datetime2_t const d)    
{
    std::stringstream ss;
    ss << "[ticks:" << d.ticks << " days:" << d.days << "]";
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
        << ") auid.id = " << int(id.d.id)
        << " hi = " << int(id.d.hi)
        ;
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
    if (1 == data.size()) {
        const auto & m = data.front();
        return std::string(m.first, m.second);
    }
    std::string s;
    s.reserve(mem_size_n(data));
    for (const auto & m : data) {
        A_STATIC_CHECK_TYPE(const mem_range_t &, m);
        s.append(m.first, m.second);
    }
    return s;
}

std::string to_string::make_ntext(vector_mem_range_t const & data)
{
    if (1 == data.size()) {
        return to_string::type(make_nchar_checked(data.front()));
    }
    std::string s;
    for (const auto & m : data) {
        A_STATIC_CHECK_TYPE(const mem_range_t &, m);
        s += to_string::type(make_nchar_checked(m));
    }
    return s;
}

bool to_string::empty_or_whitespace_text(vector_mem_range_t const & data)
{
    enum { space = ' ' };
    if (!data.empty()) {
        for (const auto & m : data) {
            A_STATIC_CHECK_TYPE(const mem_range_t &, m);
            for (const char c : m) {
                if (c != space) {
                    return false;
                }
                if (!c) {
                    SDL_ASSERT(false);
                    return true; // end of string
                }
            }
        }
    }
    return true;
}

bool to_string::empty_or_whitespace_ntext(vector_mem_range_t const & data)
{
    enum { space = ' ' };
    if (!data.empty()) {
        for (const auto & m : data) {
            A_STATIC_CHECK_TYPE(const mem_range_t &, m);
            const nchar_range range = make_nchar_checked(m);
            for (const nchar_t c : range) {
                if (c._16 != space) {
                    return false;
                }
                if (!c) {
                    SDL_ASSERT(false);
                    return true; // end of string
                }

            }
        }
    }
    return true;
}

namespace {

template<class string_type, typename string_type::value_type space>
struct impl_to_string_trim : is_static
{
    // remove leading and trailing spaces
    static string_type trim(string_type && s)
    {
        if (!s.empty()) {
            size_t const size = s.size();
            size_t s1 = 0;
            while ((s1 < size) && (s[s1] == space)) {
                ++s1;
            }
            if (s1 < size) {
                SDL_ASSERT(s[s1] != space);
                size_t s2 = size - 1;
                while (s[s2] == space) {
                    --s2;
                }
                SDL_ASSERT(s1 <= s2);
                if ((s1 == 0) && (s2 == size - 1)) {
                    return std::move(s);
                }
                return s.substr(s1, s2 - s1 + 1);
            }
        }
        return {};
    }
};

} // namespace

std::string to_string::trim(std::string && s)
{
    return impl_to_string_trim<std::string, ' '>::trim(std::move(s));
}

std::wstring to_string::trim(std::wstring && s)
{
    return impl_to_string_trim<std::wstring, L' '>::trim(std::move(s));
}

size_t to_string::length_text(vector_mem_range_t const & data) // length without trailing spaces
{
    constexpr char space = ' ';
    const size_t size = mem_size(data);
    if (size) {
        mem_range_t const * const begin = data.begin();
        mem_range_t const * p = data.end();
        SDL_ASSERT(begin < p);
        size_t trailing_spaces = 0;
        for (--p; begin <= p; --p) {
            const char * const first = p->first;
            const char * second = p->second;
            SDL_ASSERT(first < second);
            for (; first <= second; --second) {
                if (*second == space) {
                    ++trailing_spaces;
                }
                else {
                    SDL_ASSERT(trailing_spaces <= size);
                    return size - trailing_spaces;
                }
            }
        }
        SDL_ASSERT(trailing_spaces == size);
    }
    return 0;
}

size_t to_string::length_ntext(vector_mem_range_t const & data) // length without trailing spaces
{
    SDL_ASSERT(0); // not implemented
    return 0;
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

std::string to_string::type(geo_point const & p)
{
    to_string::stringstream ss;
    ss << "POINT ("         
        << p.data.point.longitude << ' '
        << p.data.point.latitude << ')';
    return ss.str();
}

std::string to_string::type(geo_linesegment const & data)
{
    to_string::stringstream ss;
    ss << "LINESTRING ("         
        << data[0].longitude << ' '
        << data[0].latitude << ", "
        << data[1].longitude << ' '
        << data[1].latitude << ')';
    return ss.str();
}

namespace {

struct type_impl : is_static {
    static std::string type_geo_pointarray(geo_pointarray const & data, const char * title, const bool polygon = false);
    static std::string type_multi_geometry(geo_mem const & data, const char * const title);
    static std::string type_MultiLineString(geo_mem const & data);
    static std::string type_MultiPolygon(geo_mem const & data);
    template<class data_type>
    static std::string type_substr_linestring(data_type const &, size_t, size_t, bool);
    static std::string type_substr_multi_geometry(geo_mem const &, size_t, size_t, bool);
    static double make_epsilon(int);
};

std::string type_impl::type_geo_pointarray(geo_pointarray const & data, const char * title, const bool polygon)
{
    to_string::stringstream ss;
    if (is_str_valid(title)) {
        ss << title << " ";
    }
    ss << "(";
    if (polygon) ss << "(";
    for (size_t i = 0; i < data.size(); ++i) {
        const auto & pt = data[i];
        if (i) {
            ss << ", ";
        }
        ss << pt.longitude << ' ' << pt.latitude;
    }
    if (polygon) ss << ")";
    ss << ")";
    return ss.str();
}

std::string type_impl::type_multi_geometry(geo_mem const & data, const char * const title)
{
    to_string::stringstream ss;
    ss << title << " (";
    const size_t numobj = data.numobj();
    SDL_ASSERT(numobj);
    for (size_t i = 0; i < numobj; ++i) {
        auto const pp = data.get_subobj(i);
        SDL_ASSERT(pp.size());
        if (i) { ss << ", "; }
        ss << "(";
        size_t count = 0;
        for (auto const & p : pp) {
            if (count++) {
                ss << ", ";
            }
            ss << p.longitude << ' ' << p.latitude;
        }
        ss << ")";
    }
    ss << ")";
    return ss.str();
}

inline double type_impl::make_epsilon(int precision) {
    SDL_ASSERT(precision > 0);
    if (precision > 0) {
        double epsilon = 1;
        while (precision--) {
            epsilon /= 10;
        }
        return epsilon;
    }
    return limits::fepsilon;
}

template<class data_type>
std::string type_impl::type_substr_linestring(data_type const & data, 
                                              const size_t pos, 
                                              const size_t count,
                                              const bool make_valid)
{
    if (count < 2) {
        SDL_ASSERT(0);
        return {};
    }
    if (pos + count <= data.size()) {
        to_string::stringstream ss;
        ss << "LINESTRING (";
        spatial_point const * p = data.begin() + pos;
        spatial_point const * const end = p + count;
        SDL_ASSERT(p < end);
        if (p < end) {
            ss << p->longitude << ' ' << p->latitude;
            if (make_valid) {
                const double epsilon = make_epsilon(to_string::precision());
                const auto start = p;
                for (auto next = p + 1; next < end; ++next) {
                    SDL_ASSERT(p < next);
                    if (!next->equal_with_epsilon(*p, epsilon)) {
                        ss << ", " << next->longitude << ' ' << next->latitude;
                        p = next;
                    }
                }
                if (p == start) {
                    SDL_WARNING(!"not valid");
                    return {};
                }
            }
            else {
                for (++p; p < end; ++p) {
                    ss << ", " << p->longitude << ' ' << p->latitude;
                }
            }
        }
        ss << ")";
        return ss.str();
    }
    SDL_ASSERT(0);
    return {};
}

std::string type_impl::type_substr_multi_geometry(geo_mem const & data, 
                                                  const size_t pos,
                                                  const size_t count,
                                                  const bool make_valid)
{
    SDL_ASSERT(data.type() == spatial_type::multilinestring);
    if (count < 2) {
        SDL_ASSERT(0);
        return {};
    }
    const size_t total = data.pointcount();
    if (total < pos + count) {
        SDL_ASSERT(0);
        return {};
    }
    const size_t numobj = data.numobj();
    SDL_ASSERT(numobj);
    size_t index_1 = pos;
    size_t index_2 = pos + count - 1;
    size_t first = numobj;
    size_t second = numobj;
    for (size_t i = 0; i < numobj; ++i) {
        size_t const sz = data.get_subobj_size(i);
        SDL_ASSERT(sz);
        if (first == numobj) {
            if (index_1 < sz) {
                first = i;
            }
            else {
                index_1 -= sz;
            }
        }
        if (index_2 < sz) {
            SDL_ASSERT(first < numobj);
            second = i;
            break;
        }
        else {
            index_2 -= sz;
        }
    }
    SDL_ASSERT(first <= second);
    SDL_ASSERT(first < numobj);
    SDL_ASSERT(second < numobj);
    if (index_1 + 1 == data.get_subobj_size(first)) {
        SDL_ASSERT(first < second);
        ++first;
        index_1 = 0;
    }
    if (index_2 == 0) {
        SDL_ASSERT(first <= second);
        SDL_ASSERT(second > 0);
        if (first < second) {
            index_2 = data.get_subobj_size(--second) - 1;
        }
        else {
            return {};
        }
    }
    if (first == second) {
        SDL_ASSERT(index_1 <= index_2);
        if (index_1 < index_2) {
            to_string::stringstream ss;
            ss << "LINESTRING (";
            auto const pp = data.get_subobj(first);
            spatial_point const * p = pp.begin() + index_1;
            spatial_point const * const end = pp.begin() + index_2 + 1;
            SDL_ASSERT(p < end);
            SDL_ASSERT(end <= pp.end());
            ss << p->longitude << ' ' << p->latitude;
            if (make_valid) {
                const double epsilon = make_epsilon(to_string::precision());
                const auto start = p;
                for (auto next = p + 1; next < end; ++next) {
                    SDL_ASSERT(p < next);
                    if (!next->equal_with_epsilon(*p, epsilon)) {
                        ss << ", " << next->longitude << ' ' << next->latitude;
                        p = next;
                    }
                }
                if (p == start) {
                    SDL_WARNING(!"not valid");
                    return {};
                }
            }
            else {
                for (++p; p < end; ++p) {
                    ss << ", " << p->longitude << ' ' << p->latitude;
                }
            }
            ss << ")";
            return ss.str();
        }
        return {};
    }
    else if (first < second) {
        if (make_valid) {
            std::string result;
            size_t subobj_count = 0;
            const double epsilon = make_epsilon(to_string::precision());
            for (size_t k = first; k <= second; ++k) {
                auto const pp = data.get_subobj(k);
                spatial_point const * p = (k == first) ? (pp.begin() + index_1) : pp.begin();
                spatial_point const * const end = (k == second) ? (pp.begin() + index_2 + 1) : pp.end();
                SDL_ASSERT(p < end);
                SDL_ASSERT(end <= pp.end());
                if (p < end) {
                    to_string::stringstream ss;
                    ss << p->longitude << ' ' << p->latitude;
                    const auto start = p;
                    for (auto next = p + 1; next != end; ++next) {
                        SDL_ASSERT(p < next);
                        if (!next->equal_with_epsilon(*p, epsilon)) {
                            ss << ", " << next->longitude << ' ' << next->latitude;
                            p = next;
                        }
                    }
                    if (start < p) {
                        if (subobj_count++) {
                            result += ", ";
                        }
                        result += "(";
                        result += ss.str();
                        result += ")";
                    }
                }
            }
            SDL_ASSERT(subobj_count <= (second - first + 1));
            if (subobj_count == 0) {
                SDL_WARNING(!"not valid");
                return {};
            }
            if (subobj_count == 1) {
                return "LINESTRING " + result;
            }
            return "MULTILINESTRING (" + result + ")";
        }
        else {
            to_string::stringstream ss;
            ss << "MULTILINESTRING (";
            size_t subobj_count = 0;
            for (size_t k = first; k <= second; ++k) {
                auto const pp = data.get_subobj(k);
                spatial_point const * p = (k == first) ? (pp.begin() + index_1) : pp.begin();
                spatial_point const * const end = (k == second) ? (pp.begin() + index_2 + 1) : pp.end();
                SDL_ASSERT(p < end);
                SDL_ASSERT(end <= pp.end());
                if (p < end) {
                    if (subobj_count++) {
                        ss << ", ";
                    }
                    ss << "(";
                    ss << p->longitude << ' ' << p->latitude;
                    for (++p; p != end; ++p) {
                        ss << ", " << p->longitude << ' ' << p->latitude;
                    }
                    ss << ")";
                }
            }
            SDL_ASSERT(subobj_count == (second - first + 1));
            ss << ")";
            return ss.str();
        }
    }
    SDL_ASSERT(0);
    return {};
} 

inline std::string type_impl::type_MultiLineString(geo_mem const & data) {
    SDL_ASSERT(data.type() == spatial_type::multilinestring);
    SDL_ASSERT(data.STGeometryType() == geometry_types::MultiLineString);
    return type_multi_geometry(data, "MULTILINESTRING");
}

std::string type_impl::type_MultiPolygon(geo_mem const & data)
{
    SDL_ASSERT(data.type() == spatial_type::multipolygon);
    if (data.STGeometryType() == geometry_types::Polygon) {
        return type_multi_geometry(data, "POLYGON");
    }
    SDL_ASSERT(data.STGeometryType() == geometry_types::MultiPolygon);
    to_string::stringstream ss;
    ss << "MULTIPOLYGON (";
    if (const auto & ring_orient = data.ring_orient()) {
        const size_t numobj = data.numobj();
        const auto & orient = *ring_orient;
        SDL_ASSERT(numobj && (orient.size() == numobj));
        if (orient.size() == numobj) {
            for (size_t i = 0; i < numobj; ++i) {
                auto const pp = data.get_subobj(i);
                SDL_ASSERT(pp.size());
                if (i) {
                    ss << ", ";
                }
                ss << (is_exterior(orient[i]) ? "((" : "(");
                size_t count = 0;
                for (auto const & p : pp) {
                    if (count++) {
                        ss << ", ";
                    }
                    ss << p.longitude << ' ' << p.latitude;
                }
                const bool last_ring_in_polygon = (i == (numobj - 1)) || is_exterior(orient[i + 1]);
                ss << (last_ring_in_polygon ? "))" : ")");
            }
        }
    }
    ss << ")";
    return ss.str();
}

} // namespace

std::string to_string::type(geo_pointarray const & data)
{
    return type_impl::type_geo_pointarray(data, "");
}

std::string to_string::type(geo_mem const & data)
{
    switch (data.type()) {
    case spatial_type::point:           return type(*data.cast_point());
    case spatial_type::linestring:      return type_impl::type_geo_pointarray(*data.cast_linestring(), "LINESTRING");
    case spatial_type::polygon:         return type_impl::type_geo_pointarray(*data.cast_polygon(), "POLYGON", true);
    case spatial_type::linesegment:     return type(*data.cast_linesegment());
    case spatial_type::multilinestring: return type_impl::type_MultiLineString(data);
    case spatial_type::multipolygon:    return type_impl::type_MultiPolygon(data);
    default:
        SDL_ASSERT(data.type() == spatial_type::null);
        break;
    }
    return{};
}

std::string to_string::type_substr(geo_mem const & data, 
    const size_t pos, const size_t count, const bool make_valid)
{
    if (count < 2) {
        SDL_ASSERT(0);
        return {};
    }
    switch (data.type()) {
    case spatial_type::linestring:
        return type_impl::type_substr_linestring(*data.cast_linestring(), pos, count, make_valid);
    case spatial_type::linesegment:
        return type_impl::type_substr_linestring(*data.cast_linesegment(), pos, count, make_valid);
    case spatial_type::multilinestring:
        return type_impl::type_substr_multi_geometry(data, pos, count, make_valid);
    default:
        SDL_ASSERT(data.type() == spatial_type::null);
        break;
    }
    return {};
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
                    SDL_ASSERT(to_string::type(d1).empty());
                    d1.days = 42003; // = SELECT DATEDIFF(d, '19000101', '20150101');
                    d1.ticks = 300;
                    //SDL_ASSERT(to_string::type(d1) == "2015-01-01 00:00:01");
                    SDL_ASSERT(to_string::type(d1) == "2015-01-01 00:00:01.000");
                    auto const ut = d1.get_unix_time();
                    const datetime_t d2 = datetime_t::set_unix_time(ut);
                    SDL_ASSERT(d1.days == d2.days);
                    SDL_ASSERT(d1.ticks == d2.ticks);
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
                        SDL_TRACE("datetime = ", to_string::type(datetime_t::set_unix_time(1474363553)));
                    }
                    {
                        SDL_ASSERT(to_string::empty_or_whitespace(""));
                        SDL_ASSERT(to_string::empty_or_whitespace(" "));
                        SDL_ASSERT(to_string::empty_or_whitespace("  "));
                        SDL_ASSERT(!to_string::empty_or_whitespace(" 1 "));
                    }
                    //FIXME: std::chrono::time_point (since C++11) 
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SDL_DEBUG


