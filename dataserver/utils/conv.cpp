// conv.cpp
//
#include "dataserver/utils/conv.h"
#include "dataserver/utils/encoding_utf.hpp"
#include "dataserver/common/locale.h"

#if defined(SDL_OS_UNIX)
#include "dataserver/utils/conv_unix.h"
#endif

//FIXME: http://www.cplusplus.com/reference/codecvt/codecvt_utf16/

namespace sdl { namespace db { namespace {

#define is_static_windows_cp1251  1
#if is_static_windows_cp1251    // setlocale = "Russian"

static const uint8 table_cp1251_to_utf8[256][4] = {
{0},{1},{2},{3},{4},{5},{6},{7},{8},{9},{10},{11},{12},{13},{14},{15},{16}, // 0..16
{17},{18},{19},{20},{21},{22},{23},{24},{25},{26},{27},{28},{29},{30},{31},{32}, // 17..32
{33},{34},{35},{36},{37},{38},{39},{40},{41},{42},{43},{44},{45},{46},{47},{48}, // 33..48
{49},{50},{51},{52},{53},{54},{55},{56},{57},{58},{59},{60},{61},{62},{63},{64}, // 49..64
{65},{66},{67},{68},{69},{70},{71},{72},{73},{74},{75},{76},{77},{78},{79},{80}, // 65..80
{81},{82},{83},{84},{85},{86},{87},{88},{89},{90},{91},{92},{93},{94},{95},{96}, // 81..96
{97},{98},{99},{100},{101},{102},{103},{104},{105},{106},{107},{108},{109},{110},{111},{112}, // 97..112
{113},{114},{115},{116},{117},{118},{119},{120},{121},{122},{123},{124},{125},{126},{127},{208,130}, // 113..128
{208,131},{226,128,154},{209,147},{226,128,158},{226,128,166},{226,128,160},{226,128,161},{226,130,172},{226,128,176},{208,137},{226,128,185},{208,138},{208,140},{208,139},{208,143},{209,146}, // 129..144
{226,128,152},{226,128,153},{226,128,156},{226,128,157},{226,128,162},{226,128,147},{226,128,148},{194,152},{226,132,162},{209,153},{226,128,186},{209,154},{209,156},{209,155},{209,159},{194,160}, // 145..160
{208,142},{209,158},{208,136},{194,164},{210,144},{194,166},{194,167},{208,129},{194,169},{208,132},{194,171},{194,172},{194,173},{194,174},{208,135},{194,176}, // 161..176
{194,177},{208,134},{209,150},{210,145},{194,181},{194,182},{194,183},{209,145},{226,132,150},{209,148},{194,187},{209,152},{208,133},{209,149},{209,151},{208,144}, // 177..192
{208,145},{208,146},{208,147},{208,148},{208,149},{208,150},{208,151},{208,152},{208,153},{208,154},{208,155},{208,156},{208,157},{208,158},{208,159},{208,160}, // 193..208
{208,161},{208,162},{208,163},{208,164},{208,165},{208,166},{208,167},{208,168},{208,169},{208,170},{208,171},{208,172},{208,173},{208,174},{208,175},{208,176}, // 209..224
{208,177},{208,178},{208,179},{208,180},{208,181},{208,182},{208,183},{208,184},{208,185},{208,186},{208,187},{208,188},{208,189},{208,190},{208,191},{209,128}, // 225..240
{209,129},{209,130},{209,131},{209,132},{209,133},{209,134},{209,135},{209,136},{209,137},{209,138},{209,139},{209,140},{209,141},{209,142},{209,143}, // 241..255
};

static const uint16 table_cp1251_to_wide[256] = {
0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16, // 0..16
17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32, // 17..32
33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48, // 33..48
49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64, // 49..64
65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80, // 65..80
81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96, // 81..96
97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112, // 97..112
113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,1026, // 113..128
1027,8218,1107,8222,8230,8224,8225,8364,8240,1033,8249,1034,1036,1035,1039,1106, // 129..144
8216,8217,8220,8221,8226,8211,8212,152,8482,1113,8250,1114,1116,1115,1119,160, // 145..160
1038,1118,1032,164,1168,166,167,1025,169,1028,171,172,173,174,1031,176, // 161..176
177,1030,1110,1169,181,182,183,1105,8470,1108,187,1112,1029,1109,1111,1040, // 177..192
1041,1042,1043,1044,1045,1046,1047,1048,1049,1050,1051,1052,1053,1054,1055,1056, // 193..208
1057,1058,1059,1060,1061,1062,1063,1064,1065,1066,1067,1068,1069,1070,1071,1072, // 209..224
1073,1074,1075,1076,1077,1078,1079,1080,1081,1082,1083,1084,1085,1086,1087,1088, // 225..240
1089,1090,1091,1092,1093,1094,1095,1096,1097,1098,1099,1100,1101,1102,1103, // 241..255
};

#endif // is_static_windows_cp1251

static std::atomic<bool> default_method_stop{false};

inline locale::conv::method_type locale_method() {
    static_assert(locale::conv::method_type::skip == locale::conv::method_type::default_method, "");
    return default_method_stop ? 
        locale::conv::method_type::stop : 
        locale::conv::method_type::skip;
}

template <class string_type>
string_type nchar_to_string(vector_mem_range_t const & data)
{
    static_assert(sizeof(nchar_t) == 2, "");
    const size_t len = mem_size(data);
    if (len && !is_odd(len)) {
        const std::vector<char> src = mem_utils::make_vector(data);
        SDL_ASSERT(src.size() == len);
        using CharIn = uint16;
        static_assert(sizeof(nchar_t) == sizeof(CharIn), "");
        const CharIn * const begin = reinterpret_cast<const CharIn *>(src.data());
        const CharIn * const end = begin + (src.size() / sizeof(CharIn));
        return sdl::locale::conv::utf_to_utf<typename string_type::value_type, CharIn>(begin, end, locale_method());
    }
    SDL_ASSERT(!len);
    return{};
}

} // namespace

bool conv::method_stop()
{
    return default_method_stop;
}

void conv::method_stop(bool b)
{
    default_method_stop = b;
}

#if is_static_windows_cp1251
std::wstring conv::cp1251_to_wide(std::string const & s)
{
    std::wstring result(s.size(), L'\0');
    size_t i = 0;
    for (char c : s) {
        result[i++] = static_cast<wchar_t>(table_cp1251_to_wide[(uint8)c]);
    }
    return result;
}

namespace {

#if SDL_DEBUG
size_t length_char_utf8(const char c) {
    size_t count = 0;
    for (const uint8 v : table_cp1251_to_utf8[(uint8)c]) {
        if (v)
            ++count;
        else
            break;
    }
    return count;
}
#endif

size_t length_utf8(std::string const & s) {
    size_t count = 0;
    for (const char c : s) {
        for (const uint8 v : table_cp1251_to_utf8[(uint8)c]) {
            if (v)
                ++count;
            else
                break;
        }
    }
    return count;
}

} // namespace

std::string conv::cp1251_to_utf8(std::string const & s)
{
    std::string result(length_utf8(s), '\0');
    size_t i = 0;
    for (char c : s) {
        for (const uint8 v : table_cp1251_to_utf8[(uint8)c]) {
            if (v) {
                SDL_ASSERT(i < result.size());
                result[i++] = static_cast<char>(v);
            }
            else
                break;
        }
    }
    return result;
}

bool conv::is_utf8(std::string const & s)
{
    for (char c : s) {
        if (static_cast<uint8>(c) > 127)
            return false;
    }
    return true;
}

#else
std::wstring conv::cp1251_to_wide(std::string const & s)
{
    std::wstring w(s.size(), L'\0');
    if (std::mbstowcs(&w[0], s.c_str(), w.size()) == s.size()) {
        return w;
    }
    SDL_ASSERT(!"cp1251_to_wide");
    return{};
}

#if defined(SDL_OS_UNIX)
std::string conv::cp1251_to_utf8(std::string const & s)
{
    return unix::iconv_cp1251_to_utf8(s);
}
#else
std::string conv::cp1251_to_utf8(std::string const & s)
{
    std::wstring w(s.size(), L'\0');
    if (std::mbstowcs(&w[0], s.c_str(), w.size()) == s.size()) {
        return sdl::locale::conv::utf_to_utf<std::string::value_type>(w, locale_method());
    }
    SDL_ASSERT(!"cp1251_to_utf8");
    return {};
}
#endif // #if defined(SDL_OS_UNIX)
#endif // is_static_windows_cp1251

std::wstring conv::utf8_to_wide(std::string const & s)
{
    if (s.empty()) 
        return {};
    return sdl::locale::conv::utf_to_utf<std::wstring::value_type>(s, locale_method());
}

std::string conv::wide_to_utf8(std::wstring const & s)
{
    if (s.empty()) 
        return {};
    return sdl::locale::conv::utf_to_utf<std::string::value_type>(s, locale_method());
}

std::string conv::nchar_to_utf8(vector_mem_range_t const & data)
{
    return nchar_to_string<std::string>(data);
}

std::wstring conv::nchar_to_wide(vector_mem_range_t const & data)
{
    return nchar_to_string<std::wstring>(data);
}

} // db
} // sdl

#if SDL_DEBUG
namespace sdl { namespace db { namespace {
    class unit_test {
    public:
        unit_test() {
            if (1) {
#if is_static_windows_cp1251 
                test_conv();
                {
                    size_t count = length_char_utf8(char(0));
                    for (uint8 i = 1; i != 0; ++i) {
                        count += length_char_utf8(char(i));
                    }
                    SDL_ASSERT(count == 401);
                }
#else
                setlocale_t::auto_locale loc("Russian");
                test_conv();
                cp1251_to_utf8();
                cp1251_to_wide();
                table_cp1251_to_utf8();
                table_cp1251_to_wide();
#endif
            }
        }
    private:
        void test_conv() {
            const auto old = conv::method_stop();
            conv::method_stop(true);
            const char * const Hello = "Привет, мир!";
            SDL_ASSERT(!conv::cp1251_to_utf8("cp1251_to_utf8").empty());
            SDL_ASSERT(!conv::cp1251_to_utf8(Hello).empty());
            SDL_ASSERT(conv::cp1251_to_utf8("").empty());
            SDL_ASSERT(!conv::cp1251_to_wide(Hello).empty());
            SDL_ASSERT(conv::cp1251_to_wide("").empty());
            conv::method_stop(old);
        }
        void cp1251_to_utf8() {
            SDL_TRACE("\ncp1251_to_utf8\n");
            for (uint8 i = 1; i != 0; ++i) {
                const char c = static_cast<char>(i);
                const std::string s1(1, c);
                const std::string s2 = conv::cp1251_to_utf8(s1);
                SDL_ASSERT(s2.size() < 4);
                std::cout 
                    << "u8[" << ((int)i) << "]" << std::hex 
                    << "[" << ((int)i) << "]" << std::dec;
                for (char c2 : s2) {
                    std::cout << "," << ((int)(uint8)(c2));
                }
                std::cout << std::endl;
            }
        }
        void table_cp1251_to_utf8() {
            SDL_TRACE("\ntable_cp1251_to_utf8\n");
            char table[256][4] = {};
            std::cout << "{0},";
            uint8 i_old = 0;
            for (uint8 i = 1; i != 0; ++i) {
                const char c = static_cast<char>(i);
                const std::string s1(1, c);
                const std::string s2 = conv::cp1251_to_utf8(s1);
                SDL_ASSERT(s2.size() < 4);
                std::cout << "{";
                for (size_t j = 0; j < s2.size(); ++j) {
                    table[i][j] = s2[j];
                    if (j) std::cout << ",";
                    std::cout << ((int)(uint8)(s2[j]));
                }
                std::cout << "},";
                if ((!(i % 16)) || (255 == i))  {
                    std::cout << " // "
                        << (int)i_old << ".."
                        << (int)i << std::endl;
                    i_old = i + 1;
                }
                SDL_ASSERT(!table[i][3]);
            }
        }
        void cp1251_to_wide() {
            SDL_TRACE("\ncp1251_to_wide\n");
            for (uint8 i = 1; i != 0; ++i) {
                const char c = static_cast<char>(i);
                const std::string s1(1, c);
                const std::wstring s2 = conv::cp1251_to_wide(s1);
                SDL_ASSERT(s2.size() < 2);
                std::cout 
                    << "u16[" << ((int)i) << "]" << std::hex 
                    << "[" << ((int)i) << "]" << std::dec;
                for (wchar_t c2 : s2) {
                    std::cout << ","
                        << ((int)(uint16)(c2));
                }
                std::cout << std::endl;
            }
        }
        void table_cp1251_to_wide() {
            SDL_TRACE("\ntable_cp1251_to_wide\n");
            uint16 table[256] = {};
            std::cout << "0,";
            uint8 i_old = 0;
            for (uint8 i = 1; i != 0; ++i) {
                const char c = static_cast<char>(i);
                const std::string s1(1, c);
                const std::wstring s2 = conv::cp1251_to_wide(s1);
                SDL_ASSERT(s2.size() == 1);
                table[i] = s2[0];
                SDL_ASSERT(table[i]);
                std::cout << ((int)(uint16)(s2[0])) << ",";
                if ((!(i % 16)) || (255 == i))  {
                    std::cout << " // "
                        << (int)i_old << ".."
                        << (int)i << std::endl;
                    i_old = i + 1;
                }
            }
        }
    };
    static unit_test s_test;
}}} // sdl::db
#endif //#if SDL_DEBUG
