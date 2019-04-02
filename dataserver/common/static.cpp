// static.cpp
//
#include "dataserver/common/static.h"

#if SDL_DEBUG
#include "dataserver/common/locale.h"
#include "dataserver/common/format.h"
#include "dataserver/common/singleton.h"
#include "dataserver/common/static_type.h"
#include "dataserver/common/bitmask.h"
#include "dataserver/common/optional.h"
#include "dataserver/common/quantity_hash.h"
#include <unordered_set>

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
    unit_test();
    void test_format_double();
};

unit_test::unit_test() {
    SDL_STATIC_ASSERT(is_odd(1));
    SDL_STATIC_ASSERT(!is_odd(2));
    static_assert(is_odd(1), "");
    static_assert(is_odd(-1), "");
    static_assert(!is_odd(2), "");
    static_assert(!is_odd(-2), "");
    static_assert(is_pod(0), "");
    static_assert(is_power_two(2), "");
    static_assert(!is_power_two(3), "");
    static_assert(sizeof(size64_t) == 8, "size64_t");
    static_assert(sizeof(size64_t) >= sizeof(size_t), "size64_t");
    static_assert((int)is_32_bit::value != (int)is_64_bit::value, "");
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
    {
        SDL_ASSERT(wstrcount(L"test %d,%d,%d", L"%") == 3);
        wchar_t buf[256] = {};
        SDL_ASSERT(!wcscmp(wide_format_s(buf, L""), L""));
        SDL_ASSERT(!wcscmp(wide_format_s(buf, L"test"), L"test"));
        memset_zero(buf);
        SDL_ASSERT(!wcscmp(wide_format_s(buf, L"test %d", 1), L"test 1"));
        memset_zero(buf);
        SDL_ASSERT(!wcscmp(wide_format_s(buf, L"test %d,%d,%d", 1, 2, 3), L"test 1,2,3"));
        memset_zero(buf);
        SDL_ASSERT(!wcscmp(wide_format_s(buf, L"%ls %d,%d,%d", L"print", 1, 2, 3), L"print 1,2,3"));
        memset_zero(buf);
        SDL_ASSERT(!wcscmp(wide_format_s(buf, L"test %d%%", 1), L"test 1%"));
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
    {
        using T = basic_quantity<unit_test, const char *>;
        T test = "test";
        T test2;
        test2 = test;
        if (0) {
            SDL_TRACE(test2);
        }
    }
    {
        using T = movable<std::vector<std::string>>;
        T x(T::value_type{"1"});
        T y(std::move(x));
        T z;
        z = std::move(y);
        SDL_ASSERT(z.value[0] == "1");
        SDL_ASSERT(x.value.empty());
        SDL_ASSERT(y.value.empty());
    }
    if (0) {
        SDL_TRACE("break_ = ", break_or_continue::break_);
        SDL_TRACE("continue_ = ", break_or_continue::continue_);
    }
    {
        optional<int> test1;
        optional<int> test2 = 0;
        SDL_ASSERT(!test1);
        SDL_ASSERT(test2);
        SDL_ASSERT(test1.value == test2.value);
        test1 = 0;
        SDL_ASSERT(test1);
        test1.reset();
        SDL_ASSERT(!test1);
    }
    {
        static_assert(binary_1<0>::value == 0, "");
        static_assert(binary_1<1>::value == 1, "");
        static_assert(binary_1<2>::value == 1, "");
        static_assert(binary_1<3>::value == 2, "");
        static_assert(binary_1<5>::value == 2, "");
        static_assert(binary_1<7>::value == 3, "");
        static_assert(binary_1<(uint8)-1>::value == 8, "");
        static_assert(binary_1<(uint32)-1>::value == 32, "");
        static_assert(binary_1<(uint64)-1>::value == 64, "");
        static_assert(binary_1<0x8000000000000000ULL>::value == 1, "");
    }
    {
        static_assert(a_square(2) == 4, "");
        static_assert(a_square(a_square(2)) == 16, "");
    }
    if (0) {
        using T = quantity<unit_test, int>;
        using hash = quantity_hash<T>;
        std::unordered_set<T, quantity_hash<T>, quantity_equal_to<T>> test;
        test.insert(T(1));
        test.insert(T(2));
        test.insert(T(1));
        SDL_ASSERT(test.size() == 2);
    }
}

void unit_test::test_format_double()
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
    SDL_ASSERT(format_double(limits::PI, 2) == "3.14");
    debug::is_unit_test() = old;
}

static unit_test s_test;

}} // sdl
#endif //#if SDL_DEBUG
