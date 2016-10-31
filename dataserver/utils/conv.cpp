// conv.cpp
//
#include "common/common.h"
#include "utils/conv.h"
#include "utils/encoding_utf.hpp"

#if defined(SDL_OS_UNIX)
#include "conv_unix.h"
#endif

namespace sdl { namespace db { namespace {

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
        const std::vector<char> src = make_vector(data);
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
#endif

std::wstring conv::utf8_to_wide(std::string const & s)
{
    A_STATIC_ASSERT_TYPE(wchar_t, std::wstring::value_type); // sizeof(wchar_t) can be 2 or 4 bytes
    return sdl::locale::conv::utf_to_utf<std::wstring::value_type>(s, locale_method());
}

std::string conv::wide_to_utf8(std::wstring const & s)
{
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
            const auto old = conv::method_stop();
            conv::method_stop(true);
            SDL_ASSERT(!conv::cp1251_to_utf8("cp1251_to_utf8").empty());
            SDL_ASSERT(conv::cp1251_to_utf8("").empty());
            conv::method_stop(old);
        }
    };
    static unit_test s_test;
}}} // sdl::db
#endif //#if SV_DEBUG
