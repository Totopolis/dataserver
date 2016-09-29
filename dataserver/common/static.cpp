// static.cpp
//
#include "common.h"
#include "static.h"

namespace sdl {

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
                SDL_ASSERT(std::string(format_double(buf, 1.23000, 10)) == "1.23");
                SDL_ASSERT(std::string(format_double(buf, -1.23000, 10)) == "-1.23");
                SDL_ASSERT(std::string(format_double(buf, 0.000, 10)) == "0");
                SDL_ASSERT(std::string(format_double(buf, -0.000, 10)) == "-0");
                debug::unit_test = old;
            }
        }
    };
    static unit_test s_test;
}} // sdl
#endif //#if SV_DEBUG
