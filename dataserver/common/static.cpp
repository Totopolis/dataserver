// static.cpp
//
#include "common.h"
#include "static.h"
#include "locale.h"
#include "utils/encoding_utf.hpp"

namespace sdl {

std::wstring cp1251_to_wide(std::string const & s)
{
    std::wstring w(s.size(), L'\0');
    if (std::mbstowcs(&w[0], s.c_str(), w.size()) == s.size()) {
        return w;
    }
    SDL_ASSERT(!"cp1251_to_wide");
    return{};
}

std::string cp1251_to_utf8(std::string const & s)
{
    std::wstring w(s.size(), L'\0');
    if (std::mbstowcs(&w[0], s.c_str(), w.size()) == s.size()) {
        return sdl::locale::conv::utf_to_utf<char>(w);
    }
    SDL_ASSERT(!"cp1251_to_utf8");
    return {};
}

} // sdl

#if SDL_DEBUG
namespace sdl { namespace {
    class unit_test {
    public:
        unit_test() {
            static_assert(is_odd(1), "");
            static_assert(is_odd(-1), "");
            static_assert(!is_odd(2), "");
            static_assert(!is_odd(-2), "");
            static_assert(is_pod(0), "");
            {
                test_format_double();
                setlocale_t::auto_locale setl("Russian");
                test_format_double();
            }
        }
        void test_format_double()
        {
            char buf[64]={};
            auto old = debug::unit_test;
            debug::unit_test = 1;
            SDL_ASSERT(std::string(format_double(buf, 1.23000123, 0)) == "1");
            SDL_ASSERT(std::string(format_double(buf, 1.23000123, 100)) == "1.23000123");
            SDL_ASSERT(std::string(format_double(buf, 1.23000123, 17)) == "1.23000123");
            SDL_ASSERT(std::string(format_double(buf, 1.23000123, 10)) == "1.23000123");
            SDL_ASSERT(std::string(format_double(buf, 1.23000123, 8)) == "1.23000123");
            SDL_ASSERT(std::string(format_double(buf, 1.23000123, 6)) == "1.230001");
            SDL_ASSERT(std::string(format_double(buf, 1.23000123, 5)) == "1.23");
            SDL_ASSERT(std::string(format_double(buf, 1.00000123, 6)) == "1.000001");
            SDL_ASSERT(std::string(format_double(buf, 1.00000123, 5)) == "1");
            SDL_ASSERT(std::string(format_double(buf, 1.23000, 10)) == "1.23");
            SDL_ASSERT(std::string(format_double(buf, -1.23000, 10)) == "-1.23");
            SDL_ASSERT(std::string(format_double(buf, 0.000, 10)) == "0");
            SDL_ASSERT(std::string(format_double(buf, -0.000, 10)) == "-0");
            debug::unit_test = old;
        }
    };
    static unit_test s_test;
}} // sdl
#endif //#if SV_DEBUG
