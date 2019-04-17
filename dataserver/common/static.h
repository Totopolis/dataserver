// static.h
//
#pragma once
#ifndef __SDL_COMMON_STATIC_H__
#define __SDL_COMMON_STATIC_H__

#include "dataserver/common/config.h"

#if defined(SDL_OS_WIN32)
#pragma warning(disable: 4996) //warning C4996: 'mbstowcs': This function or variable may be unsafe.
#endif

#if defined(SDL_OS_WIN32) && (_MSC_VER == 1800)
#error constexpr does not compiled in Visual Studio 2013 
#endif

namespace sdl {

using int8 = std::int8_t;
using uint8 = std::uint8_t;
using int16 = std::int16_t;
using uint16 = std::uint16_t;
using int32 = std::int32_t;
using uint32 = std::uint32_t;
using int64 = std::int64_t;
using uint64 = std::uint64_t;
typedef long long long_long;
typedef long long unsigned long_long_unsigned;
using llu = long_long_unsigned;

struct is_32_bit { enum { value = (sizeof(void *) == sizeof(std::uint32_t)) }; };
struct is_64_bit { enum { value = (sizeof(void *) == sizeof(std::uint64_t)) }; };

template <bool> struct size64 { 
    using type = size_t;
};
template <> struct size64<false> {
    using type = uint64;
};
using size64_t = size64<sizeof(size_t) == sizeof(uint64)>::type;

struct limits {
    limits() = delete;
    static constexpr double fepsilon = 1e-12;
    static constexpr double PI = 3.14159265358979323846;
    static constexpr double PI_2 = 2.0 * PI;
    static constexpr double RAD_TO_DEG = 57.295779513082321;
    static constexpr double DEG_TO_RAD = 0.017453292519943296;
    static constexpr double SQRT_2 = 1.41421356237309504880;        // = sqrt(2)
    static constexpr double ATAN_1_2 = 0.46364760900080609;         // = std::atan2(1, 2)
    static constexpr double EARTH_RADIUS = 6371000;                 // in meters
    static constexpr double EARTH_RADIUS_2 = 2.0 * EARTH_RADIUS;    // in meters
    static constexpr double EARTH_MAJOR_RADIUS = 6378137;           // in meters, WGS 84, Semi-major axis
    static constexpr double EARTH_MINOR_RADIUS = 6356752.314245;    // in meters, WGS 84, Semi-minor axis
    static constexpr double EARTH_MINOR_ARC = EARTH_MINOR_RADIUS * DEG_TO_RAD; // 1 degree arc in meters
    static constexpr int double_max_digits10 = std::numeric_limits<double>::max_digits10; // = 17
};

inline constexpr bool is_str_valid(const char * str)
{
    return str && str[0];
}

inline constexpr bool is_str_valid(const wchar_t * str)
{
    return str && str[0];
}

inline constexpr bool is_str_empty(const char * str)
{
    return !is_str_valid(str);
}

inline constexpr bool is_str_empty(const wchar_t * str)
{
    return !is_str_valid(str);
}

template <class T, T b> inline constexpr T a_min(const T a)
{
    static_assert(sizeof(T) <= sizeof(double), "");
    return (a < b) ? a : b;
}

template <class T, T b> inline constexpr T a_max(const T a)
{
    static_assert(sizeof(T) <= sizeof(double), "");
    return (b < a) ? a : b;
}

template <class T> inline constexpr T a_min(const T a, const T b)
{
    static_assert(sizeof(T) <= sizeof(double), "");
    return (a < b) ? a : b;
}

template <class T> inline constexpr T a_max(const T a, const T b)
{
    static_assert(sizeof(T) <= sizeof(double), "");
    return (b < a) ? a : b;
}

template <class T, T min, T max> inline constexpr T a_min_max(const T a)
{
    static_assert(sizeof(T) <= sizeof(double), "");
    static_assert(min < max, "");
    return (a < min) ? min : ((a < max) ? a : max);
}

template <class T> inline constexpr T a_min_max(const T a, const T min, const T max)
{
    static_assert(sizeof(T) <= sizeof(double), "");
    return (a < min) ? min : ((a < max) ? a : max);
}

template <class T> inline void set_min(T & a, const T b)
{
    static_assert(sizeof(T) <= sizeof(double), "");
    if (b < a) a = b;
}

template <class T> inline void set_max(T & a, const T b)
{
    static_assert(sizeof(T) <= sizeof(double), "");
    if (a < b) a = b;
}

template <class T> inline void set_max_min(T & a, const T x1, const T x2)
{
    static_assert(sizeof(T) <= sizeof(double), "");
    set_max(a, a_min(x1, x2));
}

template <class T> inline void set_min_max(T & a, const T x1, const T x2)
{
    static_assert(sizeof(T) <= sizeof(double), "");
    set_min(a, a_max(x1, x2));
}

template <class T> inline constexpr T a_abs(const T a)
{
    static_assert(sizeof(T) <= sizeof(double), "");
    return (a < 0) ? (-a) : a;
}

template <class T> inline constexpr T a_delta(const T a, const T b)
{
    static_assert(sizeof(T) <= sizeof(double), "");
    return (a < b) ? (b - a) : (a - b);
}

template <class T> inline constexpr T a_square(const T a)
{
    static_assert(sizeof(T) <= sizeof(double), "");
    return a * a;
}

template <class T> inline constexpr int a_sign(const T v)
{
    static_assert(sizeof(T) <= sizeof(double), "");
    return (v > 0) ? 1 : ((v < 0) ? -1 : 0);
}

template<class T>
inline constexpr T round_up_div(T const s, T div)
{
    return (s + div - 1) / div;
}

inline constexpr uint32 a_rotl32(const uint32 x, const uint32 r)
{
    return (x << r) | (x >> (32 - r));
}

inline constexpr bool fequal(double const f1, double const f2)
{
    return a_abs(f1 - f2) <= limits::fepsilon;
}

inline constexpr bool fequal_epsilon(double f1, double f2, double fepsilon)
{
    return a_abs(f1 - f2) <= fepsilon;
}

inline constexpr bool fpositive(double const f1)
{
    return f1 > limits::fepsilon;
}

inline SDL_NDEBUG_CONSTEXPR bool positive_fzero(double const f1) {
#if SDL_DEBUG
    SDL_ASSERT(f1 >= 0);
#endif
    return f1 <= limits::fepsilon;
}

inline constexpr bool fzero(double const f1)
{
    return a_abs(f1) <= limits::fepsilon;
}

inline constexpr bool fless_eq(double const f1, double const f2)
{
    return f1 <= (f2 + limits::fepsilon);
}

template <int f1> inline constexpr bool fless_eq(double const f2)
{
    return f1 <= (f2 + limits::fepsilon);
}

inline constexpr bool fless(double const f1, double const f2)
{
    return (f1 + limits::fepsilon) < f2;
}

inline constexpr bool frange(double const x, double const left, double const right) 
{
    return fless_eq(left, x) && fless_eq(x, right);
}

inline constexpr int fsign(double const v) {
    return (v > 0) ? 1 : ((v < 0) ? -1 : 0);
}

inline double fatan2(double const y, double const x) {
    if (fzero(y) && fzero(x))
        return 0;
    return std::atan2(y, x);
}

inline constexpr uint32 reverse_bytes(uint32 const x) {
    return ((x & 0xFF) << 24)
        | ((x & 0xFF00) << 8)
        | ((x & 0xFF0000) >> 8)
        | ((x & 0xFF000000) >> 24);
}

template<typename T>
inline constexpr bool is_odd(T const x) {
    static_assert(std::is_integral<T>::value, "is_integral");
    return (x & 1) != 0;
}

template<typename T>
inline constexpr bool is_pod(T const &) {
    return std::is_pod<T>::value;
}

template<size_t x> struct is_power_2 {
    enum { value = x && !(x & (x - 1)) };
};

template<typename T>
inline constexpr bool is_power_two(T const x) {
    return (x > 0) && !(x & (x - 1));
}

template<size_t x> 
struct power_of  {
    static_assert(is_power_2<x>::value, "power_of");
    enum { value = 1 + power_of<x/2>::value };
};

template<> struct power_of<1> {
    enum { value = 0 };
};

template<size_t N> struct kilobyte
{
    enum { value = N * (1 << 10) };
};

template<size_t N> struct megabyte // 1 MB = 2^20 = 1048,576
{
    enum { value = N * (1 << 20) };
};

template<size_t N> struct gigabyte // 1 GB = 2^30 = 1073,741,824
{
    static constexpr size64_t value = N * (size64_t(1) << 30);
};

template<size_t N> struct terabyte // 1 TB = 2^40 = 1099,511,627,776 
{ 
    static constexpr size64_t value = N * gigabyte<1024>::value;
};

template<size_t N> struct min_to_sec
{
    enum { value = N * 60 };
};

template<size_t N> struct min_to_msec
{
    enum { value = N * 60 * 1000 };
};

template<size_t N> struct hour_to_sec
{
    enum { value = N * 60 * 60 };
};

template<size_t N> struct day_to_sec
{
    enum { value = 24 * hour_to_sec<N>::value };
};

#if 0 // avoid macros conflict, use count_of()
// This template function declaration is used in defining A_ARRAY_SIZE.
// Note that the function doesn't need an implementation, as we only use its type.
template <typename T, size_t N> char(&A_ARRAY_SIZE_HELPER(T const(&array)[N]))[N];
#define A_ARRAY_SIZE(a) (sizeof(sdl::A_ARRAY_SIZE_HELPER(a)))
#endif

template <typename T> struct array_info;
template <typename T, size_t N>
struct array_info<T[N]>
{
    typedef T value_type;
    enum { size = N };
};

//Alternative of macros A_ARRAY_SIZE
//This function template can't be instantiated with pointer argument, only array.
template< class Type, size_t n > inline constexpr
size_t count_of(Type const(&)[n])
{
    return n;
}

template <uint64 N> struct binary;

template <> struct binary<0>
{
    static unsigned const value = 0;
};

template <uint64 N>
struct binary
{
    static unsigned const value = 
        (binary<N / 10>::value << 1)    // prepend higher bits
            | (N % 10);                 // to lowest bit
};

template <uint64 N> struct binary_1;
template <> struct binary_1<0> {
    enum { value = 0 };
};

template <uint64 N>
struct binary_1 {
    enum { value = 1 + binary_1<N & (N - 1)>::value }; // N > 0
};


//http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetKernighan
//Counting bits set, Brian Kernighan's way
template<typename T>
inline size_t number_of_1(T b) {
    static_assert(std::is_integral<T>::value, "");
    size_t count = 0;
    while (b) {
        b &= b - 1; // clear the least significant bit set
        ++count;
    }
    return count;
}

template<class T>
inline void memset_pod(T& dest, int value) noexcept
{
    A_STATIC_ASSERT_IS_POD(T);
    memset(&dest, value, sizeof(T));
}

template<class T> 
inline void memset_zero(T& dest) noexcept
{
    A_STATIC_ASSERT_IS_POD(T);
    memset(&dest, 0, sizeof(T));
}

template<class Dest, class Source> inline 
void memcpy_pod(Dest & dest, Source const & src) noexcept
{
    A_STATIC_ASSERT_IS_POD(Dest);
    A_STATIC_ASSERT_IS_POD(Source);
    static_assert(sizeof(Dest) == sizeof(Source), "");
    memcpy(&dest, &src, sizeof(dest));
}

template<class Type, size_t size> inline 
void memcpy_array(Type(&dest)[size], Type const(&src)[size], size_t const count) noexcept
{
    static_check_is_trivially_copyable(dest);
    static_check_is_trivially_copyable(src);
    SDL_ASSERT(count <= size);
    memcpy(&dest, &src, sizeof(Type) * a_min(count, size));
}

template<class T1, class T2> inline 
int memcmp_pod(T1 const & x, T2 const & y) noexcept
{
    A_STATIC_ASSERT_IS_POD(T1);
    A_STATIC_ASSERT_IS_POD(T2);
    static_assert(sizeof(T1) == sizeof(T2), "");
    return memcmp(&x, &y, sizeof(x));
}

inline bool memcmp_zero(uint8 const * p, uint8 const * const end) {
    SDL_ASSERT(p <= end);
    while (p != end) {
        if (*p++)
            return false;
    }
    return true;
}

inline bool memcmp_zero(char const * p, char const * const end) {
    SDL_ASSERT(p <= end);
    while (p != end) {
        if (*p++)
            return false;
    }
    return true;
}

template<class T> inline
bool memcmp_zero(T const & x) {
    A_STATIC_ASSERT_IS_POD(T);
    uint8 const * const p = reinterpret_cast<uint8 const *>(&x);
    return memcmp_zero(p, p + sizeof(x));
}

template<class T1, class T2>
inline void assign_static_cast(T1 & dest, const T2 & src) {
    dest = static_cast<T1>(src);
}

#if 0
template<class T> 
struct to_shared {
    using type = std::shared_ptr<typename T::element_type>; 
};

template<class T>
using to_shared_t = typename to_shared<T>::type;
#endif

#if 0 // std::make_unique available since C++14
template<typename T, typename... Ts> inline
std::unique_ptr<T> make_unique(Ts&&... params) {
    return std::unique_ptr<T>(new T(std::forward<Ts>(params)...));
}
#endif

template<typename T> inline
std::shared_ptr<T> make_shared(std::unique_ptr<T> && p) {
    return std::shared_ptr<T>(std::move(p));
}

template<typename T, typename... Ts> inline
void reset_new(std::unique_ptr<T> & dest, Ts&&... params) {
    dest.reset(new T(std::forward<Ts>(params)...));
}

template<typename T, typename... Ts> inline
void reset_new(std::shared_ptr<T> & dest, Ts&&... params) {
    dest = std::make_shared<T>(std::forward<Ts>(params)...);
}

template<class T, class Base, typename... Ts> inline
void reset_shared(std::shared_ptr<Base> & dest, Ts&&... params) {
    static_assert(std::is_base_of<Base, T>::value, "");
    static_assert(std::has_virtual_destructor<Base>::value, "");
    dest = std::make_shared<T>(std::forward<Ts>(params)...);
}

class sdl_exception : public std::logic_error {
    using base_type = std::logic_error;
public:
    sdl_exception(): base_type("unknown exception"){}
    explicit sdl_exception(const std::string & s): base_type(s){}
    explicit sdl_exception(const char* s): base_type(s) {
        SDL_ASSERT(s);
    }
};

template<class T>
class sdl_exception_t : public sdl_exception {
public:
    using type = T;
    explicit sdl_exception_t(const std::string & s) : sdl_exception(s){}
    explicit sdl_exception_t(const char* s) : sdl_exception(s){}
};

template<typename T, typename... Ts> inline
void throw_error(Ts&&... params) {
    static_assert(std::is_base_of<sdl_exception, T>::value, "is_base_of");
    SDL_ASSERT_WIN32(!"throw_error");
    throw T(std::forward<Ts>(params)...);
}

template<typename T, typename... Ts> inline
void throw_error_t(Ts&&... params) {
    static_assert(!std::is_base_of<sdl_exception, T>::value, "is_base_of");
    SDL_ASSERT_WIN32(!"throw_error_t");
    throw sdl_exception_t<T>(std::forward<Ts>(params)...);
}

template<typename T, typename... Ts> inline
void throw_error_if(const bool condition, Ts&&... params) {
    static_assert(std::is_base_of<sdl_exception, T>::value, "is_base_of");
    if (condition) {
        SDL_ASSERT_WIN32(!"throw_error_if");
        throw T(std::forward<Ts>(params)...);
    }
}

template<typename T, typename... Ts> inline
void throw_error_if_t(const bool condition, Ts&&... params) {
    static_assert(!std::is_base_of<sdl_exception, T>::value, "is_base_of");
    if (condition) {
        SDL_ASSERT_WIN32(!"throw_error_if_t");
        throw sdl_exception_t<T>(std::forward<Ts>(params)...);
    }
}

template<typename T, typename... Ts> inline
void throw_error_if_not(const bool condition, Ts&&... params) {
    static_assert(std::is_base_of<sdl_exception, T>::value, "is_base_of");
    if (!condition) {
        SDL_ASSERT_WIN32(!"throw_error_if_not");
        throw T(std::forward<Ts>(params)...);
    }
}

template<typename T, typename... Ts> inline
void throw_error_if_not_t(const bool condition, Ts&&... params) {
    static_assert(!std::is_base_of<sdl_exception, T>::value, "is_base_of");
    if (!condition) {
        SDL_ASSERT_WIN32(!"throw_error_if_not_t");
        throw sdl_exception_t<T>(std::forward<Ts>(params)...);
    }
}

template<typename T>
inline void sdl_throw_error(T && what) {
    SDL_ASSERT_WIN32(!"sdl_throw_error");
    throw sdl_exception(std::forward<T>(what));
}

template<typename T>
inline void sdl_throw_error_if(const bool condition, T && what) {
    if (condition) {
        SDL_ASSERT(!"sdl_throw_error_if");
        throw sdl_exception(std::forward<T>(what));
    }
}

template<class T> using vector_unique_ptr = std::vector<std::unique_ptr<T>>;
template<class T> using vector_shared_ptr = std::vector<std::shared_ptr<T>>;

template<class T>
using remove_reference_t = typename std::remove_reference<T>::type; // since C++14

template <typename T> struct identity
{
    typedef T type;
};

template<bool b> using bool_constant = std::integral_constant<bool, b>;

template<typename T> 
inline bool is_found(T && key, std::initializer_list<T> a) {
    return std::find(a.begin(), a.end(), key) != a.end();
}

enum class break_or_continue {
    break_,
    continue_
};

using bc = break_or_continue;

inline constexpr bc make_break_or_continue(bc const t) noexcept {
    return t;
}
inline constexpr bc make_break_or_continue(bool const t) noexcept {
    return static_cast<break_or_continue>(t);
}
template<class T> bc make_break_or_continue(T) = delete;

template<class T> inline constexpr bool is_break(T t) noexcept {
    return make_break_or_continue(t) == bc::break_;
}
template<class T> inline constexpr bool is_continue(T t) noexcept {
    return make_break_or_continue(t) != bc::break_;
}

inline uint32 unix_time() {
    return static_cast<uint32>(std::time(nullptr)); //https://en.wikipedia.org/wiki/Unix_time
}  

////////////////////////////////////////////////////////////////////////////////

#define SDL_DEFINE_STRINGIFY(s) #s

#if SDL_DEBUG || defined(SDL_TRACE_RELEASE)
inline std::ostream & operator <<(std::ostream & out, break_or_continue b) { // for SDL_TRACE
    out << is_continue(b);
    return out;
}
#endif

} // sdl

#endif // __SDL_COMMON_STATIC_H__
