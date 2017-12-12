// static.cpp
//
#include "dataserver/common/static.h"
#include "dataserver/common/locale.h"
#include "dataserver/common/format.h"
#include "dataserver/common/singleton.h"
#include "dataserver/common/static_type.h"

#if SDL_DEBUG
namespace sdl { namespace { // experimental

inline constexpr const char * maybe_constexpr(const char * s) { return s;}
#define is_constexpr(e) noexcept(maybe_constexpr(e))

const char * foo_runtime(const char * s){ return "foo_runtime"; }
constexpr const char * foo_compiletime(const char * s) { return "foo_compiletime"; }
#define choose_foo(X) (is_constexpr(X) ? foo_compiletime(X) : foo_runtime(X))

struct string_or_constexpr {
    std::string first;
    const char * second = nullptr;
    string_or_constexpr(const char * const s, bool_constant<0>): first(s) {} // NOT constexpr
    string_or_constexpr(const char * const s, bool_constant<1>) noexcept : second(s) {} // is constexpr
    const char * c_str() const noexcept {
        return second ? second : first.c_str();
    }
#define maybe_constexpr_arg(arg) arg, bool_constant<is_constexpr(arg)>()
};

template<typename T>
inline constexpr bool is_str_valid_t(const T * str) {
    static_assert(TL::any_is_same<T, char, signed char, unsigned char>::value, "");
    return str && str[0];
}

template<typename T>
inline constexpr bool is_str_empty_t(const T * str) {
    return !is_str_valid_t(str);
}

class unit_test {
public:
    unit_test() {
        static_assert(is_odd(1), "");
        static_assert(is_odd(-1), "");
        static_assert(!is_odd(2), "");
        static_assert(!is_odd(-2), "");
        static_assert(is_pod(0), "");
        static_assert(is_power_two(2), "");
        static_assert(!is_power_two(3), "");
        static_assert(sizeof(size64_t) == 8, "size64_t");
        static_assert(sizeof(size64_t) >= sizeof(size_t), "size64_t");
        static_assert(is_32_bit::value != is_64_bit::value, "");
        static_assert(is_power_two(size64_t(1) << 32), "");
        static_assert(!is_power_two(uint32(-1)), "");
        static_assert(is_power_two(4294967296), "");        
        static_assert(is_power_two(536870912), "");   
        static_assert(gigabyte<1>::value == 1073741824, "");
        static_assert(terabyte<1>::value == (size64_t(1) << 40), "");
        static_assert(is_str_empty(""), "");
        static_assert(is_str_empty((const char *)nullptr), "");
        static_assert(is_str_empty_t((const char *)nullptr), "");
        static_assert(is_str_empty_t((const signed char *)nullptr), "");
        static_assert(is_str_empty_t((const unsigned char *)nullptr), "");
        static_assert(TL::any_is_same<int, char, bool, int>::value, "");
        static_assert(!TL::any_is_same<int, char, bool, std::string>::value, "");
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
            memset_zero(buf);
            SDL_ASSERT(!strcmp(format_s(buf, "test %d%%", 1), "test 1%"));
        }
        SDL_TRACE_IF(false, "never see it");
        if (1) {
            const char * a = "a";
            constexpr const char * c = "c";
            if (0) {
                SDL_TRACE(a, " = ", choose_foo(a));
                SDL_TRACE(c, " = ", choose_foo(c));
                SDL_TRACE(__FUNCTION__, " = ", choose_foo(__FUNCTION__));
            }
            static_assert(!is_constexpr(a), "");
            static_assert(is_constexpr(c), "");
            static_assert(is_constexpr(__FUNCTION__), "");
            {
                const string_or_constexpr t1(a, bool_constant<is_constexpr(a)>()); SDL_ASSERT(t1.c_str() != a);
                const string_or_constexpr t2(c, bool_constant<is_constexpr(c)>()); SDL_ASSERT(t2.c_str() == c);
            }
            {
                const string_or_constexpr t1(maybe_constexpr_arg(a)); SDL_ASSERT(t1.c_str() != a);
                const string_or_constexpr t2(maybe_constexpr_arg(c)); SDL_ASSERT(t2.c_str() == c);
            }
        }
        static_assert(a_delta(1, 1000) == 999, "");
        static_assert(a_delta(1000, 1) == 999, "");
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
#endif //#if SDL_DEBUG
