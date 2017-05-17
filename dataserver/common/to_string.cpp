// to_string.cpp
//
#include "dataserver/common/to_string.h"

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
        static_assert(is_std_to_string<int>::value, "");
        static_assert(!is_std_to_string<unit_test>::value, "");
        static_assert(!is_std_to_string<test_string>::value, "");
        const std::string test = "apple";
        const auto s = str_util::to_string(1, "..", 2, "_", test, "_", test_string());
        SDL_ASSERT(s == "1..2_apple_test_string");
    }
};
static unit_test s_test;

}} // sdl

#endif //#if SDL_DEBUG




