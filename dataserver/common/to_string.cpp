// to_string.cpp
//
#include "dataserver/common/to_string.h"
#include "dataserver/common/format.h"

#if SDL_DEBUG
namespace sdl { namespace {

struct test_string {};

std::ostream & operator <<(std::ostream & out, test_string const & x) {
    out << "test_string";
    return out;
}

class unit_test {
public:
    unit_test()
    {
        {
            static_assert(is_std_to_string<int>::value, "");
            static_assert(!is_std_to_string<unit_test>::value, "");
            static_assert(!is_std_to_string<test_string>::value, "");
            const std::string test = "apple";
            const auto s = str_util::to_string(1, "..", 2, "_", test, "_", test_string());
            SDL_ASSERT(s == "1..2_apple_test_string");
        }
        {
            const char * const mask = R"(https://something/%llu/abc/%llu)";
            const auto s = to_string_format_s(mask,
                static_cast<sdl::long_long_unsigned>(1),
                static_cast<sdl::long_long_unsigned>(2));
            SDL_ASSERT(s == R"(https://something/1/abc/2)");
        }
        {
            const char mask[] = R"(https://something/%llu/abc/%llu)";
            const auto s = to_string_format_s(mask,
                static_cast<sdl::long_long_unsigned>(1),
                static_cast<sdl::long_long_unsigned>(2));
            SDL_ASSERT(s == R"(https://something/1/abc/2)");
        }
        {
            const std::string s1(100, 'a');
            const std::string s2(100, 'b');
            {
                const auto s = to_string_format_s(R"(%s+%s)", s1.c_str(), s2.c_str());
                SDL_ASSERT(s == s1 + "+" + s2);
            }
            if (0) {
                char buf[64] = "?";
                format_s(buf, R"(%s+%s)", s1.c_str(), s2.c_str()); 
                SDL_ASSERT(!buf[0]);
            }

        }
    }
};
static unit_test s_test;

}} // sdl

#endif //#if SDL_DEBUG




