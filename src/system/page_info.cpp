// page_info.cpp
//
#include "common/common.h"
#include "page_info.h"
#include <time.h>       /* time_t, struct tm, time, localtime, strftime */

namespace sdl { namespace db { namespace {

std::string type_raw_bytes(void const * _buf, size_t const buf_size)
{
    char const * buf = (char const *)_buf;
    SDL_ASSERT(buf_size);
    char xbuf[128] = {};
    std::stringstream ss;
    for (size_t i = 0; i < buf_size; ++i) {
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

std::string to_string::type(pageType const t)
{
    char buf[128];
    return std::string(format_s(buf, "%s(%d)", type_name(t), int(t.val)));
}

std::string to_string::type_raw(char const * buf, size_t const buf_size)
{
    return type_raw_buf(buf, buf_size, true);
}

std::string to_string::dump(void const * buf, size_t const buf_size)
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
        s += format_s(buf, "\nlength = %d", int(n));
        if (dump_name) {
            s += "\n\n";
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
    ss << "\nnull_bitmap = " << data.size() << "\n";
    size_t i = 0;
    for (auto v : data.copy()) {
        if (i++) ss << " ";
        ss << v;
    }
    auto s = ss.str();
    s += "\n(";
    size_t n = data.end() - data.begin();
    s += type_raw_bytes(data.begin(), n);
    s += ")";
    return s;
}

std::string to_string::type(variable_array const & data)
{
    std::stringstream ss;
    ss << "\nvariable_array = " << data.size() << "\n";
    size_t i = 0;
    for (auto v : data.copy()) {
        if (i++) ss << " ";
        ss << v;
    }
    auto s = ss.str();
    s += "\n(";
    size_t n = data.end() - data.begin();
    s += type_raw_bytes(data.begin(), n);
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
        << std::dec
        << " ("
        << id._64
        << ")";
    return ss.str();
}

std::string to_string::type(bitmask const & b)
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

std::string page_info::type_meta(page_head const & p)
{
    std::stringstream ss;
    impl::processor<page_header_meta::type_list>::print(ss, &p);
    return ss.str();
}

std::string page_info::type_raw(page_head const & p)
{
    return to_string::type_raw(p.raw);
}

std::string page_info::type_meta(record_head const & h)
{
    std::stringstream ss;
    impl::processor<record_head_meta::type_list>::print(ss, &h);
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
                        SDL_ASSERT(to_string::type(bitmask{ binary<11111111>::value }) == "255 (11111111)");
                        SDL_ASSERT(to_string::type(bitmask{ binary<10101010>::value }) == "170 (10101010)");
                        SDL_ASSERT(to_string::type(bitmask{ binary< 1010101>::value }) ==  "85 (01010101)");
                    }
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG


