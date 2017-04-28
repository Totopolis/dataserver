// static.cpp
//
#include "dataserver/common/static.h"
#include "dataserver/common/locale.h"
#include "dataserver/common/format.h"

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
            {
                SDL_ASSERT(strcount("test %d,%d,%d", "%") == 3);
                char buf[256] = {};
                SDL_ASSERT(!strcmp(format_s(buf, ""), ""));
                SDL_ASSERT(!strcmp(format_s(buf, "test"), "test"));
                memset_zero(buf);
                SDL_ASSERT(!strcmp(format_s(buf, "test %d", 1), "test 1"));
                memset_zero(buf);
                SDL_ASSERT(!strcmp(format_s(buf, "test %d,%d,%d", 1, 2, 3), "test 1,2,3"));
                memset_zero(buf);
                SDL_ASSERT(!strcmp(format_s(buf, "%s %d,%d,%d", "print", 1, 2, 3), "print 1,2,3"));
            }
            SDL_TRACE_IF(false, "never see it");
        }
        void test_format_double()
        {
            char buf[64]={};
            auto old = debug::is_unit_test();
            debug::is_unit_test() = true;
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
            debug::is_unit_test() = old;
        }
    };
    static unit_test s_test;
}} // sdl
#endif //#if SV_DEBUG
